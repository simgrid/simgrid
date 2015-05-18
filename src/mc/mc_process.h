/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PROCESS_H
#define MC_PROCESS_H

#include <stdbool.h>
#include <sys/types.h>

#include "simgrid_config.h"
#include <sys/types.h>

#include <xbt/mmalloc.h>

#ifdef HAVE_MC
#include "xbt/mmalloc/mmprivate.h"
#endif

#include <simgrid/simix.h>
#include "simix/popping_private.h"
#include "simix/smx_private.h"

#include "mc_forward.h"
#include "mc_base.h"
#include "mc_mmalloc.h" // std_heap
#include "mc_memory_map.h"
#include "AddressSpace.hpp"
#include "mc_protocol.h"

typedef int mc_process_flags_t;
#define MC_PROCESS_NO_FLAG 0
#define MC_PROCESS_SELF_FLAG 1

// Those flags are used to track down which cached information
// is still up to date and which information needs to be updated.
typedef int mc_process_cache_flags_t;
#define MC_PROCESS_CACHE_FLAG_NONE 0
#define MC_PROCESS_CACHE_FLAG_HEAP 1
#define MC_PROCESS_CACHE_FLAG_MALLOC_INFO 2
#define MC_PROCESS_CACHE_FLAG_SIMIX_PROCESSES 4

typedef struct s_mc_smx_process_info s_mc_smx_process_info_t, *mc_smx_process_info_t;

namespace simgrid {
namespace mc {

/** Representation of a process
 */
class Process : public AddressSpace {
public:
  Process(pid_t pid, int sockfd);
  ~Process();
  const void* read_bytes(void* buffer, std::size_t size,
    std::uint64_t address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) override;
public: // to be private
  mc_process_flags_t process_flags;
  pid_t pid;
  int socket;
  int status;
  bool running;
  memory_map_t memory_map;
  void *maestro_stack_start, *maestro_stack_end;
  mc_object_info_t libsimgrid_info;
  mc_object_info_t binary_info;
  mc_object_info_t* object_infos;
  size_t object_infos_size;
  int memory_file;

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
   *  use `MC_process_get_heap_info` in order to use it.
   */
   xbt_mheap_t heap;

  /** Copy of the allocation info structure
   *
   *  This is refreshed with the `MC_process_refresh` call.
   *  This is not used if the process is the current one:
   *  use `MC_process_get_malloc_info` in order to use it.
   */
  malloc_info* heap_info;

  // ***** Libunwind-data

  /** Full-featured MC-aware libunwind address space for the process
   *
   *  This address space is using a mc_unw_context_t
   *  (with mc_process_t/mc_address_space_t and unw_context_t).
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

  xbt_dynar_t checkpoint_ignore;
};

}
}

SG_BEGIN_DECL()

int MC_process_vm_open(pid_t pid, int flags);

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 */
XBT_INTERNAL void MC_process_refresh_heap(mc_process_t process);

/** Refresh the information about the process
 *
 *  Do not use direclty, this is used by the getters when appropriate
 *  in order to have fresh data.
 * */
XBT_INTERNAL void MC_process_refresh_malloc_info(mc_process_t process);

static inline
bool MC_process_is_self(mc_process_t process)
{
  return process->process_flags & MC_PROCESS_SELF_FLAG;
}

/* Process memory access: */

static inline
const void* MC_process_read(mc_process_t process,
  simgrid::mc::AddressSpace::ReadMode mode,
  void* local, const void* remote, size_t len,
  int process_index)
{
  return process->read_bytes(local, len, (std::uint64_t)remote, process_index, mode);
}

// Simplified versions/wrappers (whould be moved in mc_address_space):
XBT_INTERNAL const void* MC_process_read_simple(mc_process_t process,
  void* local, const void* remote, size_t len);
XBT_INTERNAL const void* MC_process_read_dynar_element(mc_process_t process,
  void* local, const void* remote_dynar, size_t i, size_t len);
XBT_INTERNAL unsigned long MC_process_read_dynar_length(mc_process_t process,
  const void* remote_dynar);

/** Write data to a process memory
 *
 *  @param process the process
 *  @param local   local memory address (source)
 *  @param remote  target process memory address (target)
 *  @param len     data size
 */
XBT_INTERNAL void MC_process_write(mc_process_t process,
  const void* local, void* remote, size_t len);

XBT_INTERNAL void MC_process_clear_memory(mc_process_t process,
  void* remote, size_t len);

/* Functions, variables of the process: */

XBT_INTERNAL mc_object_info_t MC_process_find_object_info(mc_process_t process, const void* addr);
XBT_INTERNAL mc_object_info_t MC_process_find_object_info_exec(mc_process_t process, const void* addr);
XBT_INTERNAL mc_object_info_t MC_process_find_object_info_rw(mc_process_t process, const void* addr);

XBT_INTERNAL dw_frame_t MC_process_find_function(mc_process_t process, const void* ip);

XBT_INTERNAL void MC_process_read_variable(mc_process_t process, const char* name, void* target, size_t size);
XBT_INTERNAL char* MC_process_read_string(mc_process_t, void* address);

static inline xbt_mheap_t MC_process_get_heap(mc_process_t process)
{
  if (!(process->cache_flags & MC_PROCESS_CACHE_FLAG_HEAP))
    MC_process_refresh_heap(process);
  return process->heap;
}

static inline malloc_info* MC_process_get_malloc_info(mc_process_t process)
{
  if (!(process->cache_flags & MC_PROCESS_CACHE_FLAG_MALLOC_INFO))
    MC_process_refresh_malloc_info(process);
  return process->heap_info;
}

/** Find (one occurence of) the named variable definition
 */
XBT_INTERNAL dw_variable_t MC_process_find_variable_by_name(mc_process_t process, const char* name);

XBT_INTERNAL void MC_invalidate_cache(void);

SG_END_DECL()

#endif
