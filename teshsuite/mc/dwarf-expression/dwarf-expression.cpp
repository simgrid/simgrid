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

#include "src/mc/mc_private.h"

#include "src/mc/Process.hpp"
#include "src/mc/Type.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/Variable.hpp"

static simgrid::mc::Process* process;

static
uintptr_t eval_binary_operation(
  simgrid::dwarf::ExpressionContext& state, int op, uintptr_t a, uintptr_t b) {

  Dwarf_Op ops[15];
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = op;

  simgrid::dwarf::ExpressionStack stack;

  try {
    simgrid::dwarf::execute(ops, 3, state, stack);
  }
  catch(std::runtime_error& e) {
    fprintf(stderr,"Expression evaluation error");
  }

  assert(stack.size() == 1);
  return stack.top();
}

static
void basic_test(simgrid::dwarf::ExpressionContext const& state) {
  try {

  Dwarf_Op ops[60];

  uintptr_t a = rand();
  uintptr_t b = rand();

  simgrid::dwarf::ExpressionStack stack;

  try {
    ops[0].atom = DW_OP_drop;
    simgrid::dwarf::execute(ops, 1, state, stack);
    fprintf(stderr,"Exception expected");
  }
  catch(simgrid::dwarf::evaluation_error& e) {}

  ops[0].atom = DW_OP_lit21;
  simgrid::dwarf::execute(ops, 1, state, stack);
  assert(stack.size() == 1);
  assert(stack.top() == 21);

  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  simgrid::dwarf::execute(ops, 1, state, stack);
  assert(stack.size() == 2);
  assert(stack.top() == a);

  ops[0].atom = DW_OP_drop;
  ops[1].atom = DW_OP_drop;
  simgrid::dwarf::execute(ops, 2, state, stack);
  assert(stack.empty());

  stack.clear();
  ops[0].atom = DW_OP_lit21;
  ops[1].atom = DW_OP_plus_uconst;
  ops[1].number = a;
  simgrid::dwarf::execute(ops, 2, state, stack);
  assert(stack.size() == 1);
  assert(stack.top() == a + 21);

  stack.clear();
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_dup;
  ops[2].atom = DW_OP_plus;
  simgrid::dwarf::execute(ops, 3, state, stack);
  assert(stack.size() == 1);
  assert(stack.top() == a + a);

  stack.clear();
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = DW_OP_over;
  simgrid::dwarf::execute(ops, 3, state, stack);
  assert(stack.size() == 3);
  assert(stack.top()  == a);
  assert(stack.top(1) == b);
  assert(stack.top(2) == a);

  stack.clear();
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = DW_OP_swap;
  simgrid::dwarf::execute(ops, 3, state, stack);
  assert(stack.size() == 2);
  assert(stack.top()  == a);
  assert(stack.top(1) == b);

  }
  catch(std::runtime_error& e) {
    fprintf(stderr,"Expression evaluation error");
  }
}

static
void test_deref(simgrid::dwarf::ExpressionContext const& state) {
  try {

  uintptr_t foo = 42;

  Dwarf_Op ops[60];
  ops[0].atom = DW_OP_const8u;
  ops[0].number = (uintptr_t) &foo;
  ops[1].atom = DW_OP_deref;

  simgrid::dwarf::ExpressionStack stack;

  simgrid::dwarf::execute(ops, 2, state, stack);
  assert(stack.size() == 1);
  assert(stack.top()  == foo);

  }
  catch(std::runtime_error& e) {
    fprintf(stderr,"Expression evaluation error");
  }
}

int main(int argc, char** argv) {
  process = new simgrid::mc::Process(getpid(), -1);
  process->init();

  simgrid::dwarf::ExpressionContext state;
  state.address_space = (simgrid::mc::AddressSpace*) process;

  basic_test(state);

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(state, DW_OP_plus, a, b) == (a + b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(state, DW_OP_or, a, b) == (a | b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(state, DW_OP_and, a, b) == (a & b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rand();
    uintptr_t b = rand();
    assert(eval_binary_operation(state, DW_OP_xor, a, b) == (a ^ b));
  }

  test_deref(state);

  return 0;
}
