/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <simgrid/modelchecker.h>

#include "src/mc/mc_record.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/Client.hpp"
#include "src/mc/ModelChecker.hpp"

/** \file mc_client_api.cpp
 *
 *  This is the implementation of the API used by the user simulated program to
 *  communicate with the MC (declared in modelchecker.h).
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client_api, mc,
  "Public API for the model-checked application");

// MC_random() is in mc_base.cpp

void MC_assert(int prop)
{
  if (MC_is_active() && !prop)
    simgrid::mc::Client::get()->reportAssertionFailure();
}

void MC_cut(void)
{
  // FIXME, We want to do this in the model-checker:
  // user_max_depth_reached = 1;
  xbt_die("MC_cut() not implemented");
}

void MC_ignore(void* addr, size_t size)
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;
  simgrid::mc::Client::get()->ignoreMemory(addr, size);
}

void MC_automaton_new_propositional_symbol(const char *id, int(*fct)(void))
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;

  xbt_die("Support for client-side function proposition is not implemented: "
    "use MC_automaton_new_propositional_symbol_pointer instead."
  );
}

void MC_automaton_new_propositional_symbol_pointer(const char *name, int* value)
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;
  simgrid::mc::Client::get()->declareSymbol(name, value);
}

/** @brief Register a stack in the model checker
 *
 *  The stacks are allocated in the heap. The MC handle them especially
 *  when we analyse/compare the content of the heap so it must be told where
 *  they are with this function.
 *
 *  @param stack
 *  @param process Process owning the stack
 *  @param context
 *  @param size    Size of the stack
 */
void MC_register_stack_area(void *stack, smx_process_t process, ucontext_t* context, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;
  simgrid::mc::Client::get()->declareStack(stack, size, process, context);
}

void MC_ignore_global_variable(const char *name)
{
  // TODO, send a message to the model_checker
  xbt_die("Unimplemented");
}

void MC_ignore_heap(void *address, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;
  simgrid::mc::Client::get()->ignoreHeap(address, size);
}

void MC_remove_ignore_heap(void *address, size_t size)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;
  simgrid::mc::Client::get()->unignoreHeap(address, size);
}
