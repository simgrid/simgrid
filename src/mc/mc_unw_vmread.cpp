/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/types.h>
#include <sys/uio.h>

#include <fcntl.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>

#include "src/mc/Process.hpp"
#include "src/mc/mc_unw.h"

/** \file
 *  Libunwind namespace implementation using process_vm_readv.
 *.
 *  This implem
 */

/** Partial structure of libunwind-ptrace context in order to get the PID
 *
 *  HACK, The context type for libunwind-race is an opaque type.
 *  We need to get the PID which is the first field. This is a hack
 *  which might break if the libunwind-ptrace structure changes.
 */
struct _UPT_info {
  pid_t pid;
  // Other things;
};

/** Get the PID of a `libunwind-ptrace` context
 */
static inline
pid_t _UPT_getpid(void* arg)
{
  struct _UPT_info* info = (_UPT_info*) arg;
  return info->pid;
}

/** Read from the memory, avoid using `ptrace` (libunwind method)
 */
static int access_mem(const unw_addr_space_t as,
              const unw_word_t addr, unw_word_t* const  valp,
              const int write, void* const arg)
{
  if (write)
    return - UNW_EINVAL;
  pid_t pid = _UPT_getpid(arg);
  size_t size = sizeof(unw_word_t);

#if HAVE_PROCESS_VM_READV
  // process_vm_read implementation.
  // This is only available since Linux 3.2.

  struct iovec local = { valp, size };
  struct iovec remote = { (void*) addr, size };
  ssize_t s = process_vm_readv(pid, &local, 1, &remote, 1, 0);
  if (s >= 0) {
    if ((size_t) s != size)
      return - UNW_EINVAL;
    else
      return 0;
  }
  if (s < 0 && errno != ENOSYS)
    return - UNW_EINVAL;
#endif

  // /proc/${pid}/mem implementation.
  // On recent kernels, we do not need to ptrace the target process.
  // On older kernels, it is necessary to ptrace the target process.
  size_t count = size;
  off_t off = (off_t) addr;
  char* buf = (char*) valp;
  int fd = simgrid::mc::open_vm(pid, O_RDONLY);
  if (fd < 0)
    return - UNW_EINVAL;
  while (1) {
    ssize_t s = pread(fd, buf, count, off);
    if (s == 0) {
      close(fd);
      return - UNW_EINVAL;
    }
    if (s == -1)
      break;
    count -= s;
    buf += s;
    off += s;
    if (count == 0) {
      close(fd);
      return 0;
    }
  }
  close(fd);

  // ptrace implementation.
  // We need to have PTRACE_ATTACH-ed it before.
  return _UPT_access_mem(as, addr, valp, write, arg);
}

namespace simgrid {
namespace unw {

/** Virtual table for our `libunwind-process_vm_readv` implementation.
 *
 *  This implementation reuse most the code of `libunwind-ptrace` but
 *  does not use ptrace() to read the target process memory by
 *  `process_vm_readv()` or `/dev/${pid}/mem` if possible.
 *
 *  Does not support any MC-specific behaviour (privatisation, snapshots)
 *  and `ucontext_t`.
 *
 *  It works with `void*` contexts allocated with `_UPT_create(pid)`.
 */
// TODO, we could get rid of this if we properly stop the model-checked
// process before reading the memory.
static unw_accessors_t accessors = {
  &_UPT_find_proc_info,
  &_UPT_put_unwind_info,
  &_UPT_get_dyn_info_list_addr,
  &access_mem,
  &_UPT_access_reg,
  &_UPT_access_fpreg,
  &_UPT_resume,
  &_UPT_get_proc_name
};

unw_addr_space_t create_addr_space()
{
  return unw_create_addr_space(&accessors, __BYTE_ORDER);
}

void* create_context(unw_addr_space_t as, pid_t pid)
{
  return _UPT_create(pid);
}

}
}