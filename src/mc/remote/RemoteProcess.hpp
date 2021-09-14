/* mc::RemoteClient: representative of the Client memory on the MC side */

/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PROCESS_H
#define SIMGRID_MC_PROCESS_H

#include "mc/datatypes.h"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/remote/RemotePtr.hpp"
#include "src/xbt/memory_map.hpp"
#include "src/xbt/mmalloc/mmprivate.h"

#include <libunwind.h>
#include <vector>

namespace simgrid {
namespace mc {

class ActorInformation {
public:
  /** MCed address of the process */
  RemotePtr<kernel::actor::ActorImpl> address{nullptr};
  Remote<kernel::actor::ActorImpl> copy;

  /** Hostname (owned by `mc_model_checker->hostnames_`) */
  const xbt::string* hostname = nullptr;
  xbt::string name;

  void clear()
  {
    name.clear();
    address  = nullptr;
    hostname = nullptr;
  }
};

struct IgnoredRegion {
  std::uint64_t addr;
  std::size_t size;
};

struct IgnoredHeapRegion {
  int block;
  int fragment;
  void* address;
  std::size_t size;
};

/** The Application's process memory, seen from the MCer perspective
 *
 *  This class is mixing a lot of different responsibilities and is tied
 *  to SIMIX. It should probably be split into different classes.
 *
 *  Responsibilities:
 *
 *  - reading from the process memory (`AddressSpace`);
 *  - accessing the system state of the process (heap, …);
 *  - storing the SIMIX state of the process;
 *  - privatization;
 *  - stack unwinding;
 *  - etc.
 */
class RemoteProcess final : public AddressSpace {
private:
  // Those flags are used to track down which cached information
  // is still up to date and which information needs to be updated.
  static constexpr int cache_none            = 0;
  static constexpr int cache_heap            = 1;
  static constexpr int cache_malloc          = 2;
  static constexpr int cache_simix_processes = 4;

public:
  explicit RemoteProcess(pid_t pid);
  ~RemoteProcess() override;
  void init(xbt_mheap_t mmalloc_default_mdp, unsigned long* maxpid, xbt_dynar_t actors, xbt_dynar_t dead_actors);

  RemoteProcess(RemoteProcess const&) = delete;
  RemoteProcess(RemoteProcess&&)      = delete;
  RemoteProcess& operator=(RemoteProcess const&) = delete;
  RemoteProcess& operator=(RemoteProcess&&) = delete;

  /* ************* */
  /* Low-level API */
  /* ************* */

  // Read memory:
  void* read_bytes(void* buffer, std::size_t size, RemotePtr<void> address,
                   ReadOptions options = ReadOptions::none()) const override;

  void read_variable(const char* name, void* target, size_t size) const;
  template <class T> void read_variable(const char* name, T* target) const
  {
    read_variable(name, target, sizeof(*target));
  }
  template <class T> Remote<T> read_variable(const char* name) const
  {
    Remote<T> res;
    read_variable(name, res.get_buffer(), sizeof(T));
    return res;
  }

  std::string read_string(RemotePtr<char> address) const;
  using AddressSpace::read_string;

  // Write memory:
  void write_bytes(const void* buffer, size_t len, RemotePtr<void> address) const;
  void clear_bytes(RemotePtr<void> address, size_t len) const;

  // Debug information:
  std::shared_ptr<ObjectInformation> find_object_info(RemotePtr<void> addr) const;
  std::shared_ptr<ObjectInformation> find_object_info_exec(RemotePtr<void> addr) const;
  std::shared_ptr<ObjectInformation> find_object_info_rw(RemotePtr<void> addr) const;
  Frame* find_function(RemotePtr<void> ip) const;
  const Variable* find_variable(const char* name) const;

  // Heap access:
  xbt_mheap_t get_heap()
  {
    if (not(this->cache_flags_ & RemoteProcess::cache_heap))
      this->refresh_heap();
    return this->heap.get();
  }
  const malloc_info* get_malloc_info()
  {
    if (not(this->cache_flags_ & RemoteProcess::cache_malloc))
      this->refresh_malloc_info();
    return this->heap_info.data();
  }

  void clear_cache() { this->cache_flags_ = RemoteProcess::cache_none; }

  std::vector<IgnoredRegion> const& ignored_regions() const { return ignored_regions_; }
  void ignore_region(std::uint64_t address, std::size_t size);

  pid_t pid() const { return pid_; }

  bool in_maestro_stack(RemotePtr<void> p) const
  {
    return p >= this->maestro_stack_start_ && p < this->maestro_stack_end_;
  }

  bool running() const { return running_; }

  void terminate() { running_ = false; }

  void ignore_global_variable(const char* name) const
  {
    for (std::shared_ptr<ObjectInformation> const& info : this->object_infos)
      info->remove_global_variable(name);
  }

  std::vector<s_stack_region_t>& stack_areas() { return stack_areas_; }
  std::vector<s_stack_region_t> const& stack_areas() const { return stack_areas_; }

  std::vector<IgnoredHeapRegion> const& ignored_heap() const { return ignored_heap_; }
  void ignore_heap(IgnoredHeapRegion const& region);
  void unignore_heap(void* address, size_t size);

  void ignore_local_variable(const char* var_name, const char* frame_name) const;

  /* ***************** */
  /* SIMIX-related API */
  /* ***************** */
private:
  // Cache the address of the variables we read directly in the memory of remote
  RemotePtr<unsigned long> maxpid_addr_;
  RemotePtr<s_xbt_dynar_t> actors_addr_;
  RemotePtr<s_xbt_dynar_t> dead_actors_addr_;

public:
  std::vector<ActorInformation>& actors();
  std::vector<ActorInformation>& dead_actors();

  /** Get a local description of a remote SIMIX actor */
  ActorInformation* resolve_actor_info(RemotePtr<kernel::actor::ActorImpl> actor)
  {
    xbt_assert(mc_model_checker != nullptr);
    if (not actor)
      return nullptr;
    this->refresh_simix();
    for (auto& actor_info : this->smx_actors_infos)
      if (actor_info.address == actor)
        return &actor_info;
    for (auto& actor_info : this->smx_dead_actors_infos)
      if (actor_info.address == actor)
        return &actor_info;
    return nullptr;
  }

  /** Get a local copy of the SIMIX actor structure */
  kernel::actor::ActorImpl* resolve_actor(RemotePtr<kernel::actor::ActorImpl> process)
  {
    ActorInformation* actor_info = this->resolve_actor_info(process);
    if (actor_info)
      return actor_info->copy.get_buffer();
    else
      return nullptr;
  }

  unsigned long get_maxpid() const { return this->read(maxpid_addr_); }

  void dump_stack() const;

private:
  void init_memory_map_info();
  void refresh_heap();
  void refresh_malloc_info();
  void refresh_simix();

  pid_t pid_    = -1;
  bool running_ = false;
  std::vector<xbt::VmMap> memory_map_;
  RemotePtr<void> maestro_stack_start_;
  RemotePtr<void> maestro_stack_end_;
  int memory_file = -1;
  std::vector<IgnoredRegion> ignored_regions_;
  std::vector<s_stack_region_t> stack_areas_;
  std::vector<IgnoredHeapRegion> ignored_heap_;

  // Copies of MCed SMX data structures
  /** Copy of `EngineImpl::actor_list_`
   *
   *  See mc_smx.cpp.
   */
  std::vector<ActorInformation> smx_actors_infos;

  /** Copy of `EngineImpl::actors_to_destroy_`
   *
   *  See mc_smx.cpp.
   */
  std::vector<ActorInformation> smx_dead_actors_infos;

  /** State of the cache (which variables are up to date) */
  int cache_flags_ = RemoteProcess::cache_none;

public:
  // object info
  // TODO, make private (first, objectify simgrid::mc::ObjectInformation*)
  std::vector<std::shared_ptr<ObjectInformation>> object_infos;
  std::shared_ptr<ObjectInformation> binary_info;

  /** Address of the heap structure in the MCed process. */
  RemotePtr<s_xbt_mheap_t> heap_address;

  /** Copy of the heap structure of the process
   *
   *  This is refreshed with the `MC_process_refresh` call.
   *  This is not used if the process is the current one:
   *  use `get_heap_info()` in order to use it.
   */
  std::unique_ptr<s_xbt_mheap_t> heap;

  /** Copy of the allocation info structure
   *
   *  This is refreshed with the `MC_process_refresh` call.
   *  This is not used if the process is the current one:
   *  use `get_malloc_info()` in order to use it.
   */
  std::vector<malloc_info> heap_info;

  // Libunwind-data
  /** Full-featured MC-aware libunwind address space for the process
   *
   *  This address space is using a simgrid::mc::UnwindContext*
   *  (with simgrid::mc::Process* / simgrid::mc::AddressSpace*
   *  and unw_context_t).
   */
  unw_addr_space_t unw_addr_space = nullptr;

  /** Underlying libunwind address-space
   *
   *  The `find_proc_info`, `put_unwind_info`, `get_dyn_info_list_addr`
   *  operations of the native MC address space is currently delegated
   *  to this address space (either the local or a ptrace unwinder).
   */
  unw_addr_space_t unw_underlying_addr_space = nullptr;

  /** The corresponding context
   */
  void* unw_underlying_context = nullptr;
};

/** Open a FD to a remote process memory (`/dev/$pid/mem`) */
XBT_PRIVATE int open_vm(pid_t pid, int flags);
} // namespace mc
} // namespace simgrid

#endif
