/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SNAPSHOT_H
#define SIMGRID_MC_SNAPSHOT_H

#include <cstdint>
#include <cstddef>

#include <vector>
#include <set>
#include <string>
#include <memory>

#include <sys/types.h> // off_t

#include <simgrid_config.h>
#include "src/xbt/mmalloc/mmprivate.h"
#include <xbt/asserts.h>
#include <xbt/base.h>

#include "src/mc/mc_forward.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/PageStore.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/mc_unw.h"
#include "src/mc/RegionSnapshot.hpp"

SG_BEGIN_DECL()

// ***** Snapshot region

XBT_PRIVATE void mc_region_restore_sparse(simgrid::mc::Process* process, mc_mem_region_t reg);

static inline __attribute__((always_inline))
void* mc_translate_address_region_chunked(uintptr_t addr, mc_mem_region_t region)
{
  auto split = simgrid::mc::mmu::split(addr - region->start().address());
  auto pageno = split.first;
  auto offset = split.second;
  const void* snapshot_page = region->page_data().page(pageno);
  return (char*) snapshot_page + offset;
}

static inline __attribute__((always_inline))
void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region, int process_index)
{
  switch (region->storage_type()) {
  case simgrid::mc::StorageType::NoData:
  default:
    xbt_die("Storage type not supported");

  case simgrid::mc::StorageType::Flat:
    {
      uintptr_t offset = (uintptr_t) addr - (uintptr_t) region->start().address();
      return (void *) ((uintptr_t) region->flat_data().get() + offset);
    }

  case simgrid::mc::StorageType::Chunked:
    return mc_translate_address_region_chunked(addr, region);

  case simgrid::mc::StorageType::Privatized:
    {
      xbt_assert(process_index >=0,
        "Missing process index for privatized region");
      xbt_assert((size_t) process_index < region->privatized_data().size(),
        "Out of range process index");
      simgrid::mc::RegionSnapshot& subregion= region->privatized_data()[process_index];
      return mc_translate_address_region(addr, &subregion, process_index);
    }
  }
}

XBT_PRIVATE mc_mem_region_t mc_get_snapshot_region(
  const void* addr, const simgrid::mc::Snapshot *snapshot, int process_index);

}

// ***** MC Snapshot

/** Ignored data
 *
 *  Some parts of the snapshot are ignored by zeroing them out: the real
 *  values is stored here.
 * */
typedef struct s_mc_snapshot_ignored_data {
  void* start;
  std::vector<char> data;
} s_mc_snapshot_ignored_data_t, *mc_snapshot_ignored_data_t;

typedef struct s_fd_infos{
  std::string filename;
  int number;
  off_t current_position;
  int flags;
}s_fd_infos_t, *fd_infos_t;

/** Information about a given stack frame
 *
 */
typedef struct s_mc_stack_frame {
  /** Instruction pointer */
  unw_word_t ip;
  /** Stack pointer */
  unw_word_t sp;
  unw_word_t frame_base;
  simgrid::mc::Frame* frame;
  std::string frame_name;
  unw_cursor_t unw_cursor;
} s_mc_stack_frame_t, *mc_stack_frame_t;

typedef struct s_local_variable{
  simgrid::mc::Frame* subprogram;
  unsigned long ip;
  std::string name;
  simgrid::mc::Type* type;
  void *address;
  int region;
} s_local_variable_t, *local_variable_t;

typedef struct XBT_PRIVATE s_mc_snapshot_stack {
  std::vector<s_local_variable> local_variables;
  simgrid::mc::UnwindContext context;
  std::vector<s_mc_stack_frame_t> stack_frames;
  int process_index;
} s_mc_snapshot_stack_t, *mc_snapshot_stack_t;

typedef struct s_mc_global_t {
  simgrid::mc::Snapshot* snapshot;
  int prev_pair;
  char *prev_req;
  int initial_communications_pattern_done;
  int recv_deterministic;
  int send_deterministic;
  char *send_diff;
  char *recv_diff;
}s_mc_global_t, *mc_global_t;

namespace simgrid {
namespace mc {

class XBT_PRIVATE Snapshot final : public AddressSpace {
public:
  Snapshot(Process* process);
  ~Snapshot();
  const void* read_bytes(void* buffer, std::size_t size,
    RemotePtr<void> address, int process_index = ProcessIndexAny,
    ReadOptions options = ReadOptions::none()) const override;
public: // To be private
  int num_state;
  std::size_t heap_bytes_used;
  std::vector<std::unique_ptr<s_mc_mem_region_t>> snapshot_regions;
  std::set<pid_t> enabled_processes;
  int privatization_index;
  std::vector<std::size_t> stack_sizes;
  std::vector<s_mc_snapshot_stack_t> stacks;
  std::vector<simgrid::mc::IgnoredHeapRegion> to_ignore;
  std::uint64_t hash;
  std::vector<s_mc_snapshot_ignored_data> ignored_data;
  std::vector<s_fd_infos_t> current_fds;
};

}
}

extern "C" {

static inline __attribute__ ((always_inline))
mc_mem_region_t mc_get_region_hinted(void* addr, simgrid::mc::Snapshot* snapshot, int process_index, mc_mem_region_t region)
{
  if (region->contain(simgrid::mc::remote(addr)))
    return region;
  else
    return mc_get_snapshot_region(addr, snapshot, process_index);
}

static const void* mc_snapshot_get_heap_end(simgrid::mc::Snapshot* snapshot);

}

#ifdef __cplusplus

namespace simgrid {
namespace mc {

XBT_PRIVATE simgrid::mc::Snapshot* take_snapshot(int num_state);
XBT_PRIVATE void restore_snapshot(simgrid::mc::Snapshot*);

}
}

#endif

extern "C" {

XBT_PRIVATE void mc_restore_page_snapshot_region(
  simgrid::mc::Process* process,
  void* start_addr, simgrid::mc::ChunkedData const& pagenos);

const void* MC_region_read_fragmented(
  mc_mem_region_t region, void* target, const void* addr, std::size_t size);

int MC_snapshot_region_memcmp(
  const void* addr1, mc_mem_region_t region1,
  const void* addr2, mc_mem_region_t region2, std::size_t size);
XBT_PRIVATE int MC_snapshot_memcmp(
  const void* addr1, simgrid::mc::Snapshot* snapshot1,
  const void* addr2, simgrid::mc::Snapshot* snapshot2, int process_index, std::size_t size);

static inline __attribute__ ((always_inline))
const void* mc_snapshot_get_heap_end(simgrid::mc::Snapshot* snapshot)
{
  if(snapshot==NULL)
      xbt_die("snapshot is NULL");
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
static inline __attribute__((always_inline))
const void* MC_region_read(
  mc_mem_region_t region, void* target, const void* addr, std::size_t size)
{
  xbt_assert(region);

  std::uintptr_t offset =
    (std::uintptr_t) addr - (std::uintptr_t) region->start().address();

  xbt_assert(region->contain(simgrid::mc::remote(addr)),
    "Trying to read out of the region boundary.");

  switch (region->storage_type()) {
  case simgrid::mc::StorageType::NoData:
  default:
    xbt_die("Storage type not supported");

  case simgrid::mc::StorageType::Flat:
    return (char*) region->flat_data().get() + offset;

  case simgrid::mc::StorageType::Chunked:
    {
      // Last byte of the region:
      void* end = (char*) addr + size - 1;
      if (simgrid::mc::mmu::sameChunk((std::uintptr_t) addr, (std::uintptr_t) end) ) {
        // The memory is contained in a single page:
        return mc_translate_address_region_chunked((uintptr_t) addr, region);
      } else {
        // The memory spans several pages:
        return MC_region_read_fragmented(region, target, addr, size);
      }
    }

  // We currently do not pass the process_index to this function so we assume
  // that the privatized region has been resolved in the callers:
  case simgrid::mc::StorageType::Privatized:
    xbt_die("Storage type not supported");
  }
}

static inline __attribute__ ((always_inline))
void* MC_region_read_pointer(mc_mem_region_t region, const void* addr)
{
  void* res;
  return *(void**) MC_region_read(region, &res, addr, sizeof(void*));
}

SG_END_DECL()

#endif
