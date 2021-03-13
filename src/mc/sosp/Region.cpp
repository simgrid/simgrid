/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/sosp/Region.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/remote/RemoteProcess.hpp"

#include <cstdlib>
#include <sys/mman.h>
#ifdef __FreeBSD__
#define MAP_POPULATE MAP_PREFAULT_READ
#endif

namespace simgrid {
namespace mc {

Region::Region(RegionType region_type, void* start_addr, size_t size)
    : region_type_(region_type), start_addr_(start_addr), size_(size)
{
  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize - 1)) == 0, "Start address not at the beginning of a page");

  chunks_ = ChunkedData(mc_model_checker->page_store(), mc_model_checker->get_remote_simulation(),
                        RemotePtr<void>(start_addr), mmu::chunk_count(size));
}

/** @brief Restore a region from a snapshot
 *
 *  @param region     Target region
 */
void Region::restore() const
{
  xbt_assert(((start().address()) & (xbt_pagesize - 1)) == 0, "Not at the beginning of a page");
  xbt_assert(simgrid::mc::mmu::chunk_count(size()) == get_chunks().page_count());

  for (size_t i = 0; i != get_chunks().page_count(); ++i) {
    void* target_page       = (void*)simgrid::mc::mmu::join(i, (std::uintptr_t)(void*)start().address());
    const void* source_page = get_chunks().page(i);
    mc_model_checker->get_remote_simulation().write_bytes(source_page, xbt_pagesize, remote(target_page));
  }
}

static XBT_ALWAYS_INLINE void* mc_translate_address_region(uintptr_t addr, const simgrid::mc::Region* region)
{
  auto split                = simgrid::mc::mmu::split(addr - region->start().address());
  auto pageno               = split.first;
  auto offset               = split.second;
  void* snapshot_page       = region->get_chunks().page(pageno);
  return (char*)snapshot_page + offset;
}

void* Region::read(void* target, const void* addr, std::size_t size) const
{
  xbt_assert(contain(simgrid::mc::remote(addr)), "Trying to read out of the region boundary.");

  // Last byte of the region:
  const void* end = (const char*)addr + size - 1;
  if (simgrid::mc::mmu::same_chunk((std::uintptr_t)addr, (std::uintptr_t)end)) {
    // The memory is contained in a single page:
    return mc_translate_address_region((uintptr_t)addr, this);
  }
  // Otherwise, the memory spans several pages. Let's copy it all into the provided buffer
  xbt_assert(target != nullptr, "Missing destination buffer for fragmented memory access");

  // TODO, we assume the chunks are aligned to natural chunk boundaries.
  // We should remove this assumption.

  // Page of the last byte of the memory area:
  size_t page_end = simgrid::mc::mmu::split((std::uintptr_t)end).first;

  void* dest = target; // iterator in the buffer to where we should copy next

  // Read each page:
  while (simgrid::mc::mmu::split((std::uintptr_t)addr).first != page_end) {
    const void* snapshot_addr = mc_translate_address_region((uintptr_t)addr, this);
    void* next_page     = (void*)simgrid::mc::mmu::join(simgrid::mc::mmu::split((std::uintptr_t)addr).first + 1, 0);
    size_t readable     = (char*)next_page - (const char*)addr;
    memcpy(dest, snapshot_addr, readable);
    addr = (const char*)addr + readable;
    dest = (char*)dest + readable;
    size -= readable;
  }

  // Read the end:
  const void* snapshot_addr = mc_translate_address_region((uintptr_t)addr, this);
  memcpy(dest, snapshot_addr, size);

  return target;
}

} // namespace mc
} // namespace simgrid

/** Compare memory between snapshots (with known regions)
 *
 * @param addr1 Address in the first snapshot
 * @param region1 Region of the address in the first snapshot
 * @param addr2 Address in the second snapshot
 * @param region2 Region of the address in the second snapshot
 * @return same semantic as memcmp
 */
int MC_snapshot_region_memcmp(const void* addr1, const simgrid::mc::Region* region1, const void* addr2,
                              const simgrid::mc::Region* region2, size_t size)
{
  // Using alloca() for large allocations may trigger stack overflow:
  // use malloc if the buffer is too big.
  bool stack_alloc    = size < 64;
  void* buffer1a      = stack_alloc ? alloca(size) : ::operator new(size);
  void* buffer2a      = stack_alloc ? alloca(size) : ::operator new(size);
  const void* buffer1 = region1->read(buffer1a, addr1, size);
  const void* buffer2 = region2->read(buffer2a, addr2, size);
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
