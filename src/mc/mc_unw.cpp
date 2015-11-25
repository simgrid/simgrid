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

#include "src/mc/mc_object_info.h"
#include "src/mc/Process.hpp"
#include "src/mc/mc_unw.h"
#include "src/mc/Frame.hpp"

using simgrid::mc::remote;

extern "C" {

// ***** Implementation

/** Get frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
static int find_proc_info(unw_addr_space_t as,
              unw_word_t ip, unw_proc_info_t *pip,
              int need_unwind_info, void* arg)
{
  mc_unw_context_t context = (mc_unw_context_t) arg;
  return unw_get_accessors(context->process->unw_underlying_addr_space)->find_proc_info(
    context->process->unw_underlying_addr_space, ip, pip,
    need_unwind_info, context->process->unw_underlying_context
  );
}

/** Release frame unwind information (libunwind method)
 *
 *  Delegates to the local/ptrace implementation.
 */
static void put_unwind_info(unw_addr_space_t as,
              unw_proc_info_t *pip, void* arg)
{
  mc_unw_context_t context = (mc_unw_context_t) arg;
  return unw_get_accessors(context->process->unw_underlying_addr_space)->put_unwind_info(
    context->process->unw_underlying_addr_space, pip,
    context->process->unw_underlying_context
  );
}

/** (libunwind method)
 *
 *  Not implemented.
 */
static int get_dyn_info_list_addr(unw_addr_space_t as,
              unw_word_t *dilap, void* arg)
{
  mc_unw_context_t context = (mc_unw_context_t) arg;
  return unw_get_accessors(context->process->unw_underlying_addr_space)->get_dyn_info_list_addr(
    context->process->unw_underlying_addr_space,
    dilap,
    context->process->unw_underlying_context
  );
}

/** Read from the target address space memory (libunwind method)
 *
 *  Delegates to the `simgrid::mc::Process*`.
 */
static int access_mem(unw_addr_space_t as,
              unw_word_t addr, unw_word_t *valp,
              int write, void* arg)
{
  mc_unw_context_t context = (mc_unw_context_t) arg;
  if (write)
    return - UNW_EREADONLYREG;
  context->address_space->read_bytes(valp, sizeof(unw_word_t), remote(addr));
  return 0;
}

static void* get_reg(unw_context_t* context, unw_regnum_t regnum)
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
  default: return NULL;
  }
#else
  return NULL;
#endif
}

/** Read a standard register (libunwind method)
 */
static int access_reg(unw_addr_space_t as,
              unw_regnum_t regnum, unw_word_t *valp,
              int write, void* arg)
{
  mc_unw_context_t as_context = (mc_unw_context_t) arg;
  unw_context_t* context = &as_context->context;
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
static int access_fpreg(unw_addr_space_t as,
              unw_regnum_t regnum, unw_fpreg_t *fpvalp,
              int write, void* arg)
{
  return -UNW_EBADREG;
}

/** Resume the execution of the context (libunwind method)
 *
 * We don't use this.
 */
static int resume(unw_addr_space_t as,
              unw_cursor_t *cp, void* arg)
{
  return -UNW_EUNSPEC;
}

/** Find informations about a function (libunwind method)
 */
static int get_proc_name(unw_addr_space_t as,
              unw_word_t addr, char *bufp,
              size_t buf_len, unw_word_t *offp,
              void* arg)
{
  mc_unw_context_t context = (mc_unw_context_t) arg;
  simgrid::mc::Frame* frame = context->process->find_function(remote(addr));
  if (!frame)
    return - UNW_ENOINFO;
  *offp = (unw_word_t) frame->low_pc - addr;

  strncpy(bufp, frame->name.c_str(), buf_len);
  if (bufp[buf_len - 1]) {
    bufp[buf_len - 1] = 0;
    return -UNW_ENOMEM;
  }

  return 0;
}

// ***** Init

unw_accessors_t mc_unw_accessors =
  {
    &find_proc_info,
    &put_unwind_info,
    &get_dyn_info_list_addr,
    &access_mem,
    &access_reg,
    &access_fpreg,
    &resume,
    &get_proc_name
  };

// ***** Context management

int mc_unw_init_context(
  mc_unw_context_t context, simgrid::mc::Process* process, unw_context_t* c)
{
  context->address_space = process;
  context->process = process;

  // Take a copy of the context for our own purpose:
  context->context = *c;
#if defined(PROCESSOR_x86_64) || defined(PROCESSOR_i686)
  // On x86_64, ucontext_t contains a pointer to itself for FP registers.
  // We don't really need support for FR registers as they are caller saved
  // and probably never use those fields as libunwind-x86_64 does not read
  // FP registers from the unw_context_t
  // but we fix the pointer in order to avoid dangling pointers:
  context->context.uc_mcontext.fpregs = &(context->context.__fpregs_mem);
#else
  // Do we need to do any fixup like this?
  #error Target CPU type is not handled.
#endif

  return 0;
}

// ***** Cursor management

int mc_unw_init_cursor(unw_cursor_t *cursor, mc_unw_context_t context)
{
  if (!context->process || !context->address_space)
    return -UNW_EUNSPEC;
  return unw_init_remote(cursor, context->process->unw_addr_space, context);
}

}
