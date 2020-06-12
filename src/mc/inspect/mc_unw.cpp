/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file
 *  Libunwind support for mc_address_space objects.
 */

// We need this for the register indices:
// #define _GNU_SOURCE

#include "src/mc/inspect/mc_unw.hpp"
#include "src/mc/inspect/Frame.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"

#include <cstring>

// On x86_64, libunwind unw_context_t has the same layout as ucontext_t:
#include <sys/types.h>
#include <sys/ucontext.h>
#ifdef __FreeBSD__
typedef register_t greg_t;
#endif

#include <libunwind.h>

using simgrid::mc::remote;

namespace simgrid {
namespace mc {

// ***** Implementation

/** Get frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
int UnwindContext::find_proc_info(unw_addr_space_t /*as*/, unw_word_t ip, unw_proc_info_t* pip, int need_unwind_info,
                                  void* arg) noexcept
{
  const simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*)arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)
      ->find_proc_info(context->process_->unw_underlying_addr_space, ip, pip, need_unwind_info,
                       context->process_->unw_underlying_context);
}

/** Release frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
void UnwindContext::put_unwind_info(unw_addr_space_t /*as*/, unw_proc_info_t* pip, void* arg) noexcept
{
  const simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*)arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)
      ->put_unwind_info(context->process_->unw_underlying_addr_space, pip, context->process_->unw_underlying_context);
}

/** (libunwind method)
 *
 *  Not implemented.
 */
int UnwindContext::get_dyn_info_list_addr(unw_addr_space_t /*as*/, unw_word_t* dilap, void* arg) noexcept
{
  const simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*)arg;
  return unw_get_accessors(context->process_->unw_underlying_addr_space)
      ->get_dyn_info_list_addr(context->process_->unw_underlying_addr_space, dilap,
                               context->process_->unw_underlying_context);
}

/** Read from the target address space memory (libunwind method)
 *
 *  Delegates to the `simgrid::mc::Process*`.
 */
int UnwindContext::access_mem(unw_addr_space_t /*as*/, unw_word_t addr, unw_word_t* valp, int write, void* arg) noexcept
{
  const simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*)arg;
  if (write)
    return -UNW_EREADONLYREG;
  context->address_space_->read_bytes(valp, sizeof(unw_word_t), remote(addr));
  return 0;
}

void* UnwindContext::get_reg(unw_context_t* context, unw_regnum_t regnum) noexcept
{
#ifdef __x86_64
  mcontext_t* mcontext = &context->uc_mcontext;
  switch (regnum) {
#ifdef __linux__
    case UNW_X86_64_RAX:
      return &mcontext->gregs[REG_RAX];
    case UNW_X86_64_RDX:
      return &mcontext->gregs[REG_RDX];
    case UNW_X86_64_RCX:
      return &mcontext->gregs[REG_RCX];
    case UNW_X86_64_RBX:
      return &mcontext->gregs[REG_RBX];
    case UNW_X86_64_RSI:
      return &mcontext->gregs[REG_RSI];
    case UNW_X86_64_RDI:
      return &mcontext->gregs[REG_RDI];
    case UNW_X86_64_RBP:
      return &mcontext->gregs[REG_RBP];
    case UNW_X86_64_RSP:
      return &mcontext->gregs[REG_RSP];
    case UNW_X86_64_R8:
      return &mcontext->gregs[REG_R8];
    case UNW_X86_64_R9:
      return &mcontext->gregs[REG_R9];
    case UNW_X86_64_R10:
      return &mcontext->gregs[REG_R10];
    case UNW_X86_64_R11:
      return &mcontext->gregs[REG_R11];
    case UNW_X86_64_R12:
      return &mcontext->gregs[REG_R12];
    case UNW_X86_64_R13:
      return &mcontext->gregs[REG_R13];
    case UNW_X86_64_R14:
      return &mcontext->gregs[REG_R14];
    case UNW_X86_64_R15:
      return &mcontext->gregs[REG_R15];
    case UNW_X86_64_RIP:
      return &mcontext->gregs[REG_RIP];
#elif defined __FreeBSD__
    case UNW_X86_64_RAX:
      return &mcontext->mc_rax;
    case UNW_X86_64_RDX:
      return &mcontext->mc_rdx;
    case UNW_X86_64_RCX:
      return &mcontext->mc_rcx;
    case UNW_X86_64_RBX:
      return &mcontext->mc_rbx;
    case UNW_X86_64_RSI:
      return &mcontext->mc_rsi;
    case UNW_X86_64_RDI:
      return &mcontext->mc_rdi;
    case UNW_X86_64_RBP:
      return &mcontext->mc_rbp;
    case UNW_X86_64_RSP:
      return &mcontext->mc_rsp;
    case UNW_X86_64_R8:
      return &mcontext->mc_r8;
    case UNW_X86_64_R9:
      return &mcontext->mc_r9;
    case UNW_X86_64_R10:
      return &mcontext->mc_r10;
    case UNW_X86_64_R11:
      return &mcontext->mc_r11;
    case UNW_X86_64_R12:
      return &mcontext->mc_r12;
    case UNW_X86_64_R13:
      return &mcontext->mc_r13;
    case UNW_X86_64_R14:
      return &mcontext->mc_r14;
    case UNW_X86_64_R15:
      return &mcontext->mc_r15;
    case UNW_X86_64_RIP:
      return &mcontext->mc_rip;
#else
#error "Unable to get register from ucontext, please add your case"
#endif
    default:
      return nullptr;
  }
#else
  return nullptr;
#endif
}

/** Read a standard register (libunwind method)
 */
int UnwindContext::access_reg(unw_addr_space_t /*as*/, unw_regnum_t regnum, unw_word_t* valp, int write,
                              void* arg) noexcept
{
  simgrid::mc::UnwindContext* as_context = (simgrid::mc::UnwindContext*)arg;
  unw_context_t* context                 = &as_context->unwind_context_;
  if (write)
    return -UNW_EREADONLYREG;
  const greg_t* preg = (greg_t*)get_reg(context, regnum);
  if (not preg)
    return -UNW_EBADREG;
  *valp = *preg;
  return 0;
}

/** Find information about a function (libunwind method)
 */
int UnwindContext::get_proc_name(unw_addr_space_t /*as*/, unw_word_t addr, char* bufp, size_t buf_len, unw_word_t* offp,
                                 void* arg) noexcept
{
  const simgrid::mc::UnwindContext* context = (simgrid::mc::UnwindContext*)arg;
  const simgrid::mc::Frame* frame           = context->process_->find_function(remote(addr));
  if (not frame)
    return -UNW_ENOINFO;
  *offp = (unw_word_t)frame->range.begin() - addr;

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
 * It works with the `simgrid::mc::UnwindContext` context.
 *
 * Use nullptr as access_fpreg and resume, as we don't need them.
 */
unw_accessors_t UnwindContext::accessors = {&find_proc_info, &put_unwind_info, &get_dyn_info_list_addr,
                                            &access_mem,     &access_reg,      nullptr,
                                            nullptr,         &get_proc_name};

unw_addr_space_t UnwindContext::createUnwindAddressSpace()
{
  return unw_create_addr_space(&UnwindContext::accessors, BYTE_ORDER);
}

void UnwindContext::initialize(simgrid::mc::RemoteSimulation* process, unw_context_t* c)
{
  this->address_space_ = process;
  this->process_      = process;

  // Take a copy of the context for our own purpose:
  this->unwind_context_ = *c;
#if SIMGRID_PROCESSOR_x86_64 || SIMGRID_PROCESSOR_i686
#ifdef __linux__
  // On x86_64, ucontext_t contains a pointer to itself for FP registers.
  // We don't really need support for FR registers as they are caller saved
  // and probably never use those fields as libunwind-x86_64 does not read
  // FP registers from the unw_context_t
  // Let's ignore this and see what happens:
  this->unwind_context_.uc_mcontext.fpregs = nullptr;
#endif
#else
  // Do we need to do any fixup like this?
#error Target CPU type is not handled.
#endif
}

unw_cursor_t UnwindContext::cursor()
{
  unw_cursor_t cursor;
  xbt_assert(process_ != nullptr && address_space_ != nullptr &&
                 unw_init_remote(&cursor, process_->unw_addr_space, this) == 0,
             "UnwindContext not initialized");
  return cursor;
}

} // namespace mc
} // namespace simgrid
