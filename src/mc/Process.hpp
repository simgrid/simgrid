/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PROCESS_H
#define SIMGRID_MC_PROCESS_H

#include <type_traits>

#include <sys/types.h>

#include <vector>
#include <memory>

#include "simgrid_config.h"
#include <sys/types.h>

#include <xbt/base.h>
#include <xbt/mmalloc.h>

#ifdef HAVE_MC
#include "src/xbt/mmalloc/mmprivate.h"
#endif

#include <simgrid/simix.h>
#include "src/simix/popping_private.h"
#include "src/simix/smx_private.h"

#include "../xbt/memory_map.hpp"

#include "mc_forward.hpp"
#include "mc_base.h"
#include "mc_mmalloc.h" // std_heap
#include "AddressSpace.hpp"
#include "mc_protocol.h"

#include "ObjectInformation.hpp"

// Those flags are used to track down which cached information
// is still up to date and which information needs to be updated.
typedef int mc_process_cache_flags_t;
#define MC_PROCESS_CACHE_FLAG_NONE 0
#define MC_PROCESS_CACHE_FLAG_HEAP 1
#define MC_PROCESS_CACHE_FLAG_MALLOC_INFO 2
#define MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES 4

namespace simgrid {
namespace mc {

struct IgnoredRegion {
  std::uint64_t addr;
  size_t size;
};

/** Representation of a process
 */
class Process final : public AddressSpace {
public:
  Process(pid_t pid, int sockfd);
  ~Process();

  Process(Process const&) = delete;
  Process(Process &&) = delete;
  Process& operator=(Process const&) = delete;
  Process& operator=(Process &&) = delete;

  // Read memory:
  const void* read_bytes(void* buffer, std::size_t size,
    remote_ptr<void> address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) const override;
  void read_variable(const char* name, void* target, size_t size) const;
  template<class T>
  T read_variable(const char *name) const
  {
    static_assert(std::is_trivial<T>::value, "Cannot read a non-trivial type");
    T res;
    read_variable(name, &res, sizeof(T));
    return res;
  }
  char* read_string(remote_ptr<void> address) const;

  // Write memory:
  void write_bytes(const void* buffer, size_t len, remote_ptr<void> address);
  void clear_bytes(remote_ptr<void> address, size_t len);

  // Debug information:
  std::shared_ptr<simgrid::mc::ObjectInformation> find_object_info(remote_ptr<void> addr) const;
  std::shared_ptr<simgrid::mc::ObjectInformation> find_object_info_exec(remote_ptr<void> addr) const;
  std::shared_ptr<simgrid::mc::ObjectInformation> find_object_info_rw(remote_ptr<void> addr) const;
  simgrid::mc::Frame* find_function(remote_ptr<void> ip) const;
  simgrid::mc::Variable* find_variable(const char* name) const;

  // Heap access:
  xbt_mheap_t get_heap()
  {
    if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP))
      this->refresh_heap();
    return this->heap.get();
  }
  malloc_info* get_malloc_info()
  {
    if (!(this->cache_flags & MC_PROCESS_CACHE_FLAG_MALLOC_INFO))
      this->refresh_malloc_info();
    return this->heap_info.data();
  }

  std::vector<IgnoredRegion> const& ignored_regions() const
  {
    return ignored_regions_;
  }
  void ignore_region(std::uint64_t address, std::size_t size);

  pid_t pid() const { return pid_; }

  bool in_maestro_stack(remote_ptr<void> p) const
  {
    return p >= this->maestro_stack_start_ && p < this->maestro_stack_end_;
  }

  bool running() const
  {
    return running_;
  }

  void terminate()
  {
    running_ = false;
  }

  template<class M>
  typename std::enable_if< std::is_class<M>::value && std::is_trivial<M>::value, int >::type
  send_message(M const& m)
  {
    return MC_protocol_send(this->socket_, &m, sizeof(M));
  }

  int send_message(e_mc_message_type message_id)
  {
    return MC_protocol_send_simple_message(this->socket_, message_id);
  }

  template<class M>
  typename std::enable_if< std::is_class<M>::value && std::is_trivial<M>::value, ssize_t >::type
  receive_message(M& m)
  {
    return MC_receive_message(this->socket_, &m, sizeof(M), 0);
  }

  void reset_soft_dirty();
  void read_pagemap(uint64_t* pagemap, size_t start_page, size_t page_count);

  bool privatized(ObjectInformation const& info) const
  {
    return privatized_ && info.executable();
  }
  bool privatized() const
  {
    return privatized_;
  }
  void privatized(bool privatized) { privatized_ = privatized; }

  void ignore_global_variable(const char* name)
  {
    for (std::shared_ptr<simgrid::mc::ObjectInformation> const& info :
        this->object_infos)
      info->remove_global_variable(name);
  }

private:
  void init_memory_map_info();
  void refresh_heap();
  void refresh_malloc_info();
private:
  pid_t pid_;
  int socket_;
  bool running_;
  std::vector<simgrid::xbt::VmMap> memory_map_;
  remote_ptr<void> maestro_stack_start_, maestro_stack_end_;
  int memory_file;
  std::vector<IgnoredRegion> ignored_regions_;
  int clear_refs_fd_;
  int pagemap_fd_;
  bool privatized_;
public: // object info
  // TODO, make private (first, objectify simgrid::mc::ObjectInformation*)
  std::vector<std::shared_ptr<simgrid::mc::ObjectInformation>> object_infos;
  std::shared_ptr<simgrid::mc::ObjectInformation> libsimgrid_info;
  std::shared_ptr<simgrid::mc::ObjectInformation> binary_info;

public: // Copies of MCed SMX data structures
  /** Copy of `simix_global->process_list`
   *
   *  See mc_smx.c.
   */
  xbt_dynar_t smx_process_infos;

  /** Copy of `simix_global->process_to_destroy`
   *
   *  See mc_smx.c.
   */
  xbt_dynar_t smx_old_process_infos;

  /** State of the cache (which variables are up to date) */
  mc_process_cache_flags_t cache_flags;

  /** Address of the heap structure in the MCed process. */
  void* heap_address;

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

public: // Libunwind-data

  /** Full-featured MC-aware libunwind address space for the process
   *
   *  This address space is using a mc_unw_context_t
   *  (with simgrid::mc::Process* / simgrid::mc::AddressSpace*
   *  and unw_context_t).
   */
  unw_addr_space_t unw_addr_space;

  /** Underlying libunwind addres-space
   *
   *  The `find_proc_info`, `put_unwind_info`, `get_dyn_info_list_addr`
   *  operations of the native MC address space is currently delegated
   *  to this address space (either the local or a ptrace unwinder).
   */
  unw_addr_space_t unw_underlying_addr_space;

  /** The corresponding context
   */
  void* unw_underlying_context;
};

/** Open a FD to a remote process memory (`/dev/$pid/mem`)
 */
XBT_PRIVATE int open_vm(pid_t pid, int flags);

}
}

#endif
