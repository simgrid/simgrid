/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mc/mc_private.h"
#include "mc/mc_object_info.h"

#include "mc/Process.hpp"
#include "mc/Type.hpp"
#include "mc/ObjectInformation.hpp"
#include "mc/Variable.hpp"

static simgrid::mc::Process* process;

static
uintptr_t eval_binary_operation(mc_expression_state_t state, int op, uintptr_t a, uintptr_t b) {
  state->stack_size = 0;

  Dwarf_Op ops[15];
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = op;

  assert(mc_dwarf_execute_expression(3, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==1);
  return state->stack[state->stack_size - 1];
}

static
void basic_test(mc_expression_state_t state) {
  Dwarf_Op ops[60];

  uintptr_t a = rand();
  uintptr_t b = rand();

  ops[0].atom = DW_OP_drop;
  assert(mc_dwarf_execute_expression(1, ops, state) == MC_EXPRESSION_E_STACK_UNDERFLOW);

  ops[0].atom = DW_OP_lit21;
  assert(mc_dwarf_execute_expression(1, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==1);
  assert(state->stack[state->stack_size-1]==21);

  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  assert(mc_dwarf_execute_expression(1, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==2);
  assert(state->stack[state->stack_size-1] == a);

  ops[0].atom = DW_OP_drop;
  ops[1].atom = DW_OP_drop;
  assert(mc_dwarf_execute_expression(2, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==0);

  ops[0].atom = DW_OP_lit21;
  ops[1].atom = DW_OP_plus_uconst;
  ops[1].number = a;
  assert(mc_dwarf_execute_expression(2, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==1);
  assert(state->stack[state->stack_size-1]== a + 21);

  state->stack_size = 0;
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_dup;
  ops[2].atom = DW_OP_plus;
  assert(mc_dwarf_execute_expression(3, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==1);
  assert(state->stack[state->stack_size-1]== a + a);

  state->stack_size = 0;
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = DW_OP_over;
  assert(mc_dwarf_execute_expression(3, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==3);
  assert(state->stack[state->stack_size-1]== a);
  assert(state->stack[state->stack_size-2]== b);
  assert(state->stack[state->stack_size-3]== a);

  state->stack_size = 0;
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = DW_OP_swap;
  assert(mc_dwarf_execute_expression(3, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size=2);
  assert(state->stack[state->stack_size-1]== a);
  assert(state->stack[state->stack_size-2]== b);
}

static
void test_deref(mc_expression_state_t state) {
  uintptr_t foo = 42;

  Dwarf_Op ops[60];
  ops[0].atom = DW_OP_const8u;
  ops[0].number = (uintptr_t) &foo;
  ops[1].atom = DW_OP_deref;
  state->stack_size = 0;

  assert(mc_dwarf_execute_expression(2, ops, state) == MC_EXPRESSION_OK);
  assert(state->stack_size==1);
  assert(state->stack[state->stack_size-1] == foo);
}

int main(int argc, char** argv) {
  process = new simgrid::mc::Process(getpid(), -1);

  s_mc_expression_state_t state;
  memset(&state, 0, sizeof(s_mc_expression_state_t));
  state.address_space = (simgrid::mc::AddressSpace*) process;

  basic_test(&state);

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(&state, DW_OP_plus, a, b) == (a + b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(&state, DW_OP_or, a, b) == (a | b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(&state, DW_OP_and, a, b) == (a & b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(&state, DW_OP_xor, a, b) == (a ^ b));
  }

  test_deref(&state);

  return 0;
}
