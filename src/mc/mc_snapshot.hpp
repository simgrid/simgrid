/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SNAPSHOT_HPP
#define SIMGRID_MC_SNAPSHOT_HPP

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "src/mc/ModelChecker.hpp"
#include "src/mc/RegionSnapshot.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_unw.hpp"

extern "C" {

// ***** Snapshot region

XBT_PRIVATE void mc_region_restore_sparse(simgrid::mc::RemoteClient* process, mc_mem_region_t reg);

static XBT_ALWAYS_INLINE void* mc_translate_address_region_chunked(uintptr_t addr, mc_mem_region_t region)
{
  auto split                = simgrid::mc::mmu::split(addr - region->start().address());
  auto pageno               = split.first;
  auto offset               = split.second;
  const void* snapshot_page = region->page_data().page(pageno);
  return (char*)snapshot_page + offset;
}

static XBT_ALWAYS_INLINE void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region, int process_index)
{
  switch (region->storage_type()) {
    case simgrid::mc::StorageType::Flat: {
      uintptr_t offset = (uintptr_t)addr - (uintptr_t)region->start().address();
      return (void*)((uintptr_t)region->flat_data().get() + offset);
    }
    case simgrid::mc::StorageType::Chunked:
      return mc_translate_address_region_chunked(addr, region);
    case simgrid::mc::StorageType::Privatized: {
      xbt_assert(process_index >= 0, "Missing process index for privatized region");
      xbt_assert((size_t)process_index < region->privatized_data().size(), "Out of range process index");
      simgrid::mc::RegionSnapshot& subregion = region->privatized_data()[process_index];
      return mc_translate_address_region(addr, &subregion, process_index);
    }
    default: // includes StorageType::NoData
      xbt_die("Storage type not supported");
  }
}

XBT_PRIVATE mc_mem_region_t mc_get_snapshot_region(const void* addr, const simgrid::mc::Snapshot* snapshot,
                                                   int process_index);
}

// ***** MC Snapshot

/** Ignored data
 *
 *  Some parts of the snapshot are ignored by zeroing them out: the real
 *  values is stored here.
 * */
struct s_mc_snapshot_ignored_data_t {
  void* start;
  std::vector<char> data;
};

struct s_fd_infos_t {
  std::string filename;
  int number;
  off_t current_position;
  int flags;
};

/** Information about a given stack frame */
struct s_mc_stack_frame_t {
  /** Instruction pointer */
  unw_word_t ip;
  /** Stack pointer */
  unw_word_t sp;
  unw_word_t frame_base;
  simgrid::mc::Frame* frame;
  std::string frame_name;
  unw_cursor_t unw_cursor;
};
typedef s_mc_stack_frame_t* mc_stack_frame_t;

struct s_local_variable_t {
  simgrid::mc::Frame* subprogram;
  unsigned long ip;
  std::string name;
  simgrid::mc::Type* type;
  void* address;
  int region;
};
typedef s_local_variable_t* local_variable_t;

struct XBT_PRIVATE s_mc_snapshot_stack_t {
  std::vector<s_local_variable_t> local_variables;
  simgrid::mc::UnwindContext context;
  std::vector<s_mc_stack_frame_t> stack_frames;
  int process_index;
};
typedef s_mc_snapshot_stack_t* mc_snapshot_stack_t;

namespace simgrid {
namespace mc {

class XBT_PRIVATE Snapshot final : public AddressSpace {
public:
  Snapshot(RemoteClient* process, int num_state);
  ~Snapshot() = default;
  const void* read_bytes(void* buffer, std::size_t size, RemotePtr<void> address, int process_index = ProcessIndexAny,
                         ReadOptions options = ReadOptions::none()) const override;

  // To be private
  int num_state;
  std::size_t heap_bytes_used;
  std::vector<std::unique_ptr<s_mc_mem_region_t>> snapshot_regions;
  std::set<pid_t> enabled_processes;
  int privatization_index;
  std::vector<std::size_t> stack_sizes;
  std::vector<s_mc_snapshot_stack_t> stacks;
  std::vector<simgrid::mc::IgnoredHeapRegion> to_ignore;
  std::uint64_t hash;
  std::vector<s_mc_snapshot_ignored_data_t> ignored_data;
  std::vector<s_fd_infos_t> current_fds;
};
}
}

extern "C" {

static XBT_ALWAYS_INLINE mc_mem_region_t mc_get_region_hinted(void* addr, simgrid::mc::Snapshot* snapshot,
                                                              int process_index, mc_mem_region_t region)
{
  if (region->contain(simgrid::mc::remote(addr)))
    return region;
  else
    return mc_get_snapshot_region(addr, snapshot, process_index);
}

static const void* mc_snapshot_get_heap_end(simgrid::mc::Snapshot* snapshot);
}

namespace simgrid {
namespace mc {

XBT_PRIVATE std::shared_ptr<simgrid::mc::Snapshot> take_snapshot(int num_state);
XBT_PRIVATE void restore_snapshot(std::shared_ptr<simgrid::mc::Snapshot> snapshot);
}
}

extern "C" {

XBT_PRIVATE void mc_restore_page_snapshot_region(simgrid::mc::RemoteClient* process, void* start_addr,
                                                 simgrid::mc::ChunkedData const& pagenos);

const void* MC_region_read_fragmented(mc_mem_region_t region, void* target, const void* addr, std::size_t size);

int MC_snapshot_region_memcmp(const void* addr1, mc_mem_region_t region1, const void* addr2, mc_mem_region_t region2,
                              std::size_t size);
XBT_PRIVATE int MC_snapshot_memcmp(const void* addr1, simgrid::mc::Snapshot* snapshot1, const void* addr2,
                                   simgrid::mc::Snapshot* snapshot2, int process_index, std::size_t size);

static XBT_ALWAYS_INLINE const void* mc_snapshot_get_heap_end(simgrid::mc::Snapshot* snapshot)
{
  if (snapshot == nullptr)
    xbt_die("snapshot is nullptr");
  return mc_model_checker->process().get_heap()->breakval;
}

/** @brief Read memory from a snapshot region
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
static XBT_ALWAYS_INLINE const void* MC_region_read(mc_mem_region_t region, void* target, const void* addr,
                                                    std::size_t size)
{
  xbt_assert(region);

  std::uintptr_t offset = (std::uintptr_t)addr - (std::uintptr_t)region->start().address();

  xbt_assert(region->contain(simgrid::mc::remote(addr)), "Trying to read out of the region boundary.");

  switch (region->storage_type()) {
    case simgrid::mc::StorageType::Flat:
      return (char*)region->flat_data().get() + offset;

    case simgrid::mc::StorageType::Chunked: {
      // Last byte of the region:
      void* end = (char*)addr + size - 1;
      if (simgrid::mc::mmu::sameChunk((std::uintptr_t)addr, (std::uintptr_t)end)) {
        // The memory is contained in a single page:
        return mc_translate_address_region_chunked((uintptr_t)addr, region);
      }
      // Otherwise, the memory spans several pages:
      return MC_region_read_fragmented(region, target, addr, size);
    }

    default:
      // includes StorageType::NoData and StorageType::Privatized (we currently do not pass the process_index to this
      // function so we assume that the privatized region has been resolved in the callers)
      xbt_die("Storage type not supported");
  }
}

static XBT_ALWAYS_INLINE void* MC_region_read_pointer(mc_mem_region_t region, const void* addr)
{
  void* res;
  return *(void**)MC_region_read(region, &res, addr, sizeof(void*));
}
}

#endif
