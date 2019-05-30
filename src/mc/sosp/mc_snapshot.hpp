/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SNAPSHOT_HPP
#define SIMGRID_MC_SNAPSHOT_HPP

#include "src/mc/ModelChecker.hpp"
#include "src/mc/inspect/mc_unw.hpp"
#include "src/mc/remote/RemoteClient.hpp"
#include "src/mc/sosp/RegionSnapshot.hpp"

// ***** Snapshot region

static XBT_ALWAYS_INLINE void* mc_translate_address_region(uintptr_t addr, simgrid::mc::RegionSnapshot* region)
{
  auto split                = simgrid::mc::mmu::split(addr - region->start().address());
  auto pageno               = split.first;
  auto offset               = split.second;
  const void* snapshot_page = region->page_data().page(pageno);
  return (char*)snapshot_page + offset;
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
};
typedef s_local_variable_t* local_variable_t;

struct XBT_PRIVATE s_mc_snapshot_stack_t {
  std::vector<s_local_variable_t> local_variables;
  simgrid::mc::UnwindContext context;
  std::vector<s_mc_stack_frame_t> stack_frames;
};
typedef s_mc_snapshot_stack_t* mc_snapshot_stack_t;

namespace simgrid {
namespace mc {

class XBT_PRIVATE Snapshot final : public AddressSpace {
public:
  Snapshot(int num_state, RemoteClient* process = &mc_model_checker->process());
  ~Snapshot() = default;

  /* Initialization */

  /* Regular use */
  const void* read_bytes(void* buffer, std::size_t size, RemotePtr<void> address,
                         ReadOptions options = ReadOptions::none()) const override;
  RegionSnapshot* get_region(const void* addr) const;
  RegionSnapshot* get_region(const void* addr, RegionSnapshot* hinted_region) const;
  void restore(RemoteClient* process);

  // To be private
  int num_state_;
  std::size_t heap_bytes_used_;
  std::vector<std::unique_ptr<RegionSnapshot>> snapshot_regions_;
  std::set<pid_t> enabled_processes_;
  std::vector<std::size_t> stack_sizes_;
  std::vector<s_mc_snapshot_stack_t> stacks_;
  std::vector<simgrid::mc::IgnoredHeapRegion> to_ignore_;
  std::uint64_t hash_ = 0;
  std::vector<s_mc_snapshot_ignored_data_t> ignored_data_;

private:
  void add_region(RegionType type, ObjectInformation* object_info, void* start_addr, void* permanent_addr,
                  std::size_t size);
  void snapshot_regions(simgrid::mc::RemoteClient* process);
  void snapshot_stacks(simgrid::mc::RemoteClient* process);
};
} // namespace mc
} // namespace simgrid

static const void* mc_snapshot_get_heap_end(simgrid::mc::Snapshot* snapshot);

namespace simgrid {
namespace mc {

XBT_PRIVATE std::shared_ptr<Snapshot> take_snapshot(int num_state);
} // namespace mc
} // namespace simgrid

const void* MC_region_read_fragmented(simgrid::mc::RegionSnapshot* region, void* target, const void* addr,
                                      std::size_t size);

int MC_snapshot_region_memcmp(const void* addr1, simgrid::mc::RegionSnapshot* region1, const void* addr2,
                              simgrid::mc::RegionSnapshot* region2, std::size_t size);

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
static XBT_ALWAYS_INLINE const void* MC_region_read(simgrid::mc::RegionSnapshot* region, void* target, const void* addr,
                                                    std::size_t size)
{
  xbt_assert(region);

  xbt_assert(region->contain(simgrid::mc::remote(addr)), "Trying to read out of the region boundary.");

      // Last byte of the region:
      void* end = (char*)addr + size - 1;
      if (simgrid::mc::mmu::same_chunk((std::uintptr_t)addr, (std::uintptr_t)end)) {
        // The memory is contained in a single page:
        return mc_translate_address_region((uintptr_t)addr, region);
      }
      // Otherwise, the memory spans several pages:
      return MC_region_read_fragmented(region, target, addr, size);
}

static XBT_ALWAYS_INLINE void* MC_region_read_pointer(simgrid::mc::RegionSnapshot* region, const void* addr)
{
  void* res;
  return *(void**)MC_region_read(region, &res, addr, sizeof(void*));
}

#endif
