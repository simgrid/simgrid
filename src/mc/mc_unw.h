/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UNW_H
#define SIMGRID_MC_UNW_H

/** \file
 *  Libunwind implementation for the model-checker
 *
 *  Libunwind provides an pluggable stack unwinding API: the way the current
 *  registers and memory is accessed, the way unwinding informations is found
 *  is pluggable.
 *
 *  This component implements the libunwind API for he model-checker:
 *
 *    * reading memory from a simgrid::mc::AddressSpace*;
 *
 *    * reading stack registers from a saved snapshot (context).
 *
 *  Parts of the libunwind information fetching is currently handled by the
 *  standard `libunwind` implementations (either the local one or the ptrace one)
 *  because parsing `.eh_frame` section is not fun and `libdw` does not help
 *  much here.
 */

#include <xbt/base.h>

#include <libunwind.h>

#include "src/mc/Process.hpp"

SG_BEGIN_DECL()

// ***** Libunwind namespace

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
extern XBT_PRIVATE unw_accessors_t mc_unw_vmread_accessors;

/** Virtual table for our `libunwind` implementation
 *
 *  Stack unwinding on a `simgrid::mc::Process*` (for memory, unwinding information)
 *  and `ucontext_t` (for processor registers).
 *
 *  It works with the `s_mc_unw_context_t` context.
 */
extern XBT_PRIVATE unw_accessors_t mc_unw_accessors;

// ***** Libunwind context

/** A `libunwind` context
 */
typedef struct XBT_PRIVATE s_mc_unw_context {
  simgrid::mc::AddressSpace* address_space;
  simgrid::mc::Process*       process;
  unw_context_t      context;
} s_mc_unw_context_t, *mc_unw_context_t;

/** Initialises an already allocated context */
XBT_PRIVATE int mc_unw_init_context(
  mc_unw_context_t context, simgrid::mc::Process* process, unw_context_t* c);

// ***** Libunwind cursor

/** Initialises a `libunwind` cursor */
XBT_PRIVATE int mc_unw_init_cursor(unw_cursor_t *cursor, mc_unw_context_t context);

SG_END_DECL()

#endif
