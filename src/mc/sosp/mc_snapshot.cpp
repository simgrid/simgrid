/* Copyright (c) 2014-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>

#include <memory>
#include <utility>

#include "xbt/asserts.h"
#include "xbt/sysdep.h"

#include "src/internal_config.h"
#include "src/smpi/include/private.hpp"

#include "src/mc/mc_mmu.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/PageStore.hpp"
#include "src/mc/sosp/mc_snapshot.hpp"


/** @brief Read memory from a snapshot region broken across fragmented pages
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
const void* MC_region_read_fragmented(simgrid::mc::RegionSnapshot* region, void* target, const void* addr, size_t size)
{
  // Last byte of the memory area:
  void* end = (char*)addr + size - 1;

  // TODO, we assume the chunks are aligned to natural chunk boundaries.
  // We should remove this assumption.

  // Page of the last byte of the memory area:
  size_t page_end = simgrid::mc::mmu::split((std::uintptr_t)end).first;

  void* dest = target;

  if (dest == nullptr)
    xbt_die("Missing destination buffer for fragmented memory access");

  // Read each page:
  while (simgrid::mc::mmu::split((std::uintptr_t)addr).first != page_end) {
    void* snapshot_addr = mc_translate_address_region_chunked((uintptr_t)addr, region);
    void* next_page     = (void*)simgrid::mc::mmu::join(simgrid::mc::mmu::split((std::uintptr_t)addr).first + 1, 0);
    size_t readable     = (char*)next_page - (char*)addr;
    memcpy(dest, snapshot_addr, readable);
    addr = (char*)addr + readable;
    dest = (char*)dest + readable;
    size -= readable;
  }

  // Read the end:
  void* snapshot_addr = mc_translate_address_region_chunked((uintptr_t)addr, region);
  memcpy(dest, snapshot_addr, size);

  return target;
}

/** Compare memory between snapshots (with known regions)
 *
 * @param addr1 Address in the first snapshot
 * @param region1 Region of the address in the first snapshot
 * @param addr2 Address in the second snapshot
 * @param region2 Region of the address in the second snapshot
 * @return same semantic as memcmp
 */
int MC_snapshot_region_memcmp(const void* addr1, simgrid::mc::RegionSnapshot* region1, const void* addr2,
                              simgrid::mc::RegionSnapshot* region2, size_t size)
{
  // Using alloca() for large allocations may trigger stack overflow:
  // use malloc if the buffer is too big.
  bool stack_alloc = size < 64;
  void* buffer1a   = nullptr;
  void* buffer2a   = nullptr;
  if (region1 != nullptr && region1->storage_type() != simgrid::mc::StorageType::Flat)
    buffer1a = stack_alloc ? alloca(size) : ::operator new(size);
  if (region2 != nullptr && region2->storage_type() != simgrid::mc::StorageType::Flat)
    buffer2a = stack_alloc ? alloca(size) : ::operator new(size);
  const void* buffer1 = MC_region_read(region1, buffer1a, addr1, size);
  const void* buffer2 = MC_region_read(region2, buffer2a, addr2, size);
  int res;
  if (buffer1 == buffer2)
    res = 0;
  else
    res = memcmp(buffer1, buffer2, size);
  if (not stack_alloc) {
    ::operator delete(buffer1a);
    ::operator delete(buffer2a);
  }
  return res;
}

namespace simgrid {
namespace mc {

Snapshot::Snapshot(RemoteClient* process, int _num_state)
    : AddressSpace(process)
    , num_state_(_num_state)
    , heap_bytes_used_(0)
    , enabled_processes_()
    , privatization_index_(0)
    , hash_(0)
{
}

void Snapshot::add_region(RegionType type, ObjectInformation* object_info, void* start_addr, void* permanent_addr,
                          std::size_t size)
{
  if (type == simgrid::mc::RegionType::Data)
    xbt_assert(object_info, "Missing object info for object.");
  else if (type == simgrid::mc::RegionType::Heap)
    xbt_assert(not object_info, "Unexpected object info for heap region.");

  simgrid::mc::RegionSnapshot* region;
#if HAVE_SMPI
  const bool privatization_aware = object_info && mc_model_checker->process().privatized(*object_info);
  if (privatization_aware && MC_smpi_process_count())
    region = new RegionPrivatized(type, start_addr, permanent_addr, size);
  else
#endif
    region = simgrid::mc::region(type, start_addr, permanent_addr, size);

  region->object_info(object_info);
  snapshot_regions_.push_back(std::unique_ptr<simgrid::mc::RegionSnapshot>(std::move(region)));
}

const void* Snapshot::read_bytes(void* buffer, std::size_t size, RemotePtr<void> address, int process_index,
                                 ReadOptions options) const
{
  RegionSnapshot* region = this->get_region((void*)address.address(), process_index);
  if (region) {
    const void* res = MC_region_read(region, buffer, (void*)address.address(), size);
    if (buffer == res || options & ReadOptions::lazy())
      return res;
    else {
      memcpy(buffer, res, size);
      return buffer;
    }
  } else
    return this->process()->read_bytes(buffer, size, address, process_index, options);
}
/** @brief Find the snapshoted region from a pointer
 *
 *  @param addr     Pointer
 *  @param process_index rank requesting the region
 * */
RegionSnapshot* Snapshot::get_region(const void* addr, int process_index) const
{
  size_t n = snapshot_regions_.size();
  for (size_t i = 0; i != n; ++i) {
    RegionSnapshot* region = snapshot_regions_[i].get();
    if (not(region && region->contain(simgrid::mc::remote(addr))))
      continue;

    if (region->storage_type() == simgrid::mc::StorageType::Privatized) {
#if HAVE_SMPI
      // Use the current process index of the snapshot:
      if (process_index == simgrid::mc::ProcessIndexDisabled)
        process_index = privatization_index_;
      xbt_assert(process_index >= 0, "Missing process index");
      xbt_assert(process_index < (int)region->privatized_data().size(), "Invalid process index");

      RegionSnapshot* priv_region = region->privatized_data()[process_index].get();
      xbt_assert(priv_region->contain(simgrid::mc::remote(addr)));
      return priv_region;
#else
      xbt_die("Privatized region in a non SMPI build (this should not happen)");
#endif
    }

    return region;
  }

  return nullptr;
}

/** @brief Find the snapshoted region from a pointer, with a hinted_region */
RegionSnapshot* Snapshot::get_region(const void* addr, int process_index, RegionSnapshot* hinted_region) const
{
  if (hinted_region->contain(simgrid::mc::remote(addr)))
    return hinted_region;
  else
    return get_region(addr, process_index);
}

} // namespace mc
} // namespace simgrid
