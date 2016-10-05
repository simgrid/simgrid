/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file
 *  Libunwind support for mc_address_space objects.
 */

// We need this for the register indices:
// #define _GNU_SOURCE

#include <string.h>

// On x86_64, libunwind unw_context_t has the same layout as ucontext_t:
#include <sys/ucontext.h>

#include <libunwind.h>

#include "src/mc/Process.hpp"
#include "src/mc/mc_unw.h"
#include "src/mc/Frame.hpp"

using simgrid::mc::remote;

namespace simgrid {
namespace mc {

// ***** Implementation

/** Get frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
int UnwindContext::find_proc_info(unw_addr_space_t as,
              unw_word_t ip, unw_proc_info_t *pip,
              int need_unwind_info, void* arg) noexcept
{
  simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*) arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)->find_proc_info(
    context->process_->unw_underlying_addr_space, ip, pip,
    need_unwind_info, context->process_->unw_underlying_context
  );
}

/** Release frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
void UnwindContext::put_unwind_info(unw_addr_space_t as,
              unw_proc_info_t *pip, void* arg) noexcept
{
  simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*) arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)->put_unwind_info(
    context->process_->unw_underlying_addr_space, pip,
    context->process_->unw_underlying_context
  );
}

/** (libunwind method)
 *
 *  Not implemented.
 */
int UnwindContext::get_dyn_info_list_addr(unw_addr_space_t as,
              unw_word_t *dilap, void* arg) noexcept
{
  simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*) arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)->get_dyn_info_list_addr(
    context->process_->unw_underlying_addr_space,
    dilap,
    context->process_->unw_underlying_context
  );
}

/** Read from the target address space memory (libunwind method)
 *
 *  Delegates to the `simgrid::mc::Process*`.
 */
int UnwindContext::access_mem(unw_addr_space_t as,
              unw_word_t addr, unw_word_t *valp,
              int write, void* arg) noexcept
{
  simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*) arg;
  if (write)
    return - UNW_EREADONLYREG;
  context->addressSpace_->read_bytes(valp, sizeof(unw_word_t), remote(addr));
  return 0;
}

void* UnwindContext::get_reg(unw_context_t* context, unw_regnum_t regnum) noexcept
{
#ifdef __x86_64
  mcontext_t* mcontext = &context->uc_mcontext;
  switch (regnum) {
  case UNW_X86_64_RAX: return &mcontext->gregs[REG_RAX];
  case UNW_X86_64_RDX: return &mcontext->gregs[REG_RDX];
  case UNW_X86_64_RCX: return &mcontext->gregs[REG_RCX];
  case UNW_X86_64_RBX: return &mcontext->gregs[REG_RBX];
  case UNW_X86_64_RSI: return &mcontext->gregs[REG_RSI];
  case UNW_X86_64_RDI: return &mcontext->gregs[REG_RDI];
  case UNW_X86_64_RBP: return &mcontext->gregs[REG_RBP];
  case UNW_X86_64_RSP: return &mcontext->gregs[REG_RSP];
  case UNW_X86_64_R8:  return &mcontext->gregs[REG_R8];
  case UNW_X86_64_R9:  return &mcontext->gregs[REG_R9];
  case UNW_X86_64_R10: return &mcontext->gregs[REG_R10];
  case UNW_X86_64_R11: return &mcontext->gregs[REG_R11];
  case UNW_X86_64_R12: return &mcontext->gregs[REG_R12];
  case UNW_X86_64_R13: return &mcontext->gregs[REG_R13];
  case UNW_X86_64_R14: return &mcontext->gregs[REG_R14];
  case UNW_X86_64_R15: return &mcontext->gregs[REG_R15];
  case UNW_X86_64_RIP: return &mcontext->gregs[REG_RIP];
  default: return nullptr;
  }
#else
  return nullptr;
#endif
}

/** Read a standard register (libunwind method)
 */
int UnwindContext::access_reg(unw_addr_space_t as,
              unw_regnum_t regnum, unw_word_t *valp,
              int write, void* arg) noexcept
{
  simgrid::mc::UnwindContext* as_context = (simgrid::mc::UnwindContext*) arg;
  unw_context_t* context = &as_context->unwindContext_;
  if (write)
    return -UNW_EREADONLYREG;
  greg_t* preg = (greg_t*) get_reg(context, regnum);
  if (!preg)
    return -UNW_EBADREG;
  *valp = *preg;
  return 0;
}

/** Read a floating-point register (libunwind method)
 *
 *  FP registers are caller-saved. The values saved by functions such as
 *  `getcontext()` is not relevant for the caller. It is not really necessary
 *  to save and handle them.
 */
int UnwindContext::access_fpreg(unw_addr_space_t as,
              unw_regnum_t regnum, unw_fpreg_t *fpvalp,
              int write, void* arg) noexcept
{
  return -UNW_EBADREG;
}

/** Resume the execution of the context (libunwind method)
 *
 * We don't use this.
 */
int UnwindContext::resume(unw_addr_space_t as,
              unw_cursor_t *cp, void* arg) noexcept
{
  return -UNW_EUNSPEC;
}

/** Find informations about a function (libunwind method)
 */
int UnwindContext::get_proc_name(unw_addr_space_t as,
              unw_word_t addr, char *bufp,
              size_t buf_len, unw_word_t *offp,
              void* arg) noexcept
{
  simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*) arg;
  simgrid::mc::Frame* frame = context->process_->find_function(remote(addr));
  if (!frame)
    return - UNW_ENOINFO;
  *offp = (unw_word_t) frame->range.begin() - addr;

  strncpy(bufp, frame->name.c_str(), buf_len);
  if (bufp[buf_len - 1]) {
    bufp[buf_len - 1] = 0;
    return -UNW_ENOMEM;
  }

  return 0;
}

// ***** Init

/** Virtual table for our `libunwind` implementation
 *
 *  Stack unwinding on a `simgrid::mc::Process*` (for memory, unwinding information)
 *  and `ucontext_t` (for processor registers).
 *
 *  It works with the `simgrid::mc::UnwindContext` context.
 */
unw_accessors_t UnwindContext::accessors = {
  &find_proc_info,
  &put_unwind_info,
  &get_dyn_info_list_addr,
  &access_mem,
  &access_reg,
  &access_fpreg,
  &resume,
  &get_proc_name
};

unw_addr_space_t UnwindContext::createUnwindAddressSpace()
{
  return unw_create_addr_space(&UnwindContext::accessors, __BYTE_ORDER);
}

void UnwindContext::clear()
{
  addressSpace_ = nullptr;
  process_ = nullptr;
}

void UnwindContext::initialize(simgrid::mc::Process* process, unw_context_t* c)
{
  clear();

  this->addressSpace_ = process;
  this->process_ = process;

  // Take a copy of the context for our own purpose:
  this->unwindContext_ = *c;
#if SIMGRID_PROCESSOR_x86_64 || SIMGRID_PROCESSOR_i686
  // On x86_64, ucontext_t contains a pointer to itself for FP registers.
  // We don't really need support for FR registers as they are caller saved
  // and probably never use those fields as libunwind-x86_64 does not read
  // FP registers from the unw_context_t
  // but we fix the pointer in order to avoid dangling pointers:
  // context->context_.uc_mcontext.fpregs = &(context->context.__fpregs_mem);

  // Let's ignore this and see what happens:
  this->unwindContext_.uc_mcontext.fpregs = nullptr;
#else
  // Do we need to do any fixup like this?
  #error Target CPU type is not handled.
#endif
}

unw_cursor_t UnwindContext::cursor()
{
  unw_cursor_t cursor;
  if (process_ == nullptr
    || addressSpace_ == nullptr
    || unw_init_remote(&cursor, process_->unw_addr_space, this) != 0)
    xbt_die("UnwindContext not initialized");
  return cursor;
}

}
}
