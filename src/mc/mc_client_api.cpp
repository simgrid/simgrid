/* Copyright (c) 2008-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_ignore.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/remote/AppSide.hpp"
#include "xbt/asserts.h"

/** @file mc_client_api.cpp
 *
 *  This is the implementation of the API used by the user simulated program to
 *  communicate with the MC (declared in modelchecker.h).
 */

// MC_random() is in mc_base.cpp

void MC_assert(int prop)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not prop) {
    if (MC_is_active())
      simgrid::mc::AppSide::get()->report_assertion_failure();
    if (MC_record_replay_is_active())
      xbt_die("MC assertion failed");
  }
}

void MC_cut()
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  // FIXME, We want to do this in the model-checker:
  xbt_die("MC_cut() not implemented");
}

void MC_ignore(void* addr, size_t size)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  simgrid::mc::AppSide::get()->ignore_memory(addr, size);
}

void MC_automaton_new_propositional_symbol(const char* /*id*/, int (*/*fct*/)())
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  xbt_die("Support for client-side function proposition is not implemented: "
    "use MC_automaton_new_propositional_symbol_pointer instead.");
}

void MC_automaton_new_propositional_symbol_pointer(const char *name, int* value)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  simgrid::mc::AppSide::get()->declare_symbol(name, value);
}

/** @brief Register a stack in the model checker
 *
 *  The stacks are allocated in the heap. The MC handle them specifically
 *  when we analyze/compare the content of the heap so it must be told where
 *  they are with this function.
 *
 *  @param stack Where the stack is
 *  @param actor Actor owning the stack
 *  @param context The context associated to that stack
 *  @param size    Size of the stack
 */
void MC_register_stack_area(void* stack, ucontext_t* context, size_t size)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  simgrid::mc::AppSide::get()->declare_stack(stack, size, context);
}

void MC_ignore_global_variable(const char* /*name*/)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  // TODO, send a message to the model_checker
  xbt_die("Unimplemented");
}

void MC_ignore_heap(void *address, size_t size)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  simgrid::mc::AppSide::get()->ignore_heap(address, size);
}

void MC_unignore_heap(void* address, size_t size)
{
  xbt_assert(mc_model_checker == nullptr);
  if (not MC_is_active())
    return;
  simgrid::mc::AppSide::get()->unignore_heap(address, size);
}
