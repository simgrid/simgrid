/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "src/mc/mc_private.hpp"

#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/Type.hpp"
#include "src/mc/inspect/Variable.hpp"
#include "src/mc/remote/RemoteProcess.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <limits>
#include <xbt/random.hpp>

static uintptr_t rnd_engine()
{
  return simgrid::xbt::random::uniform_int(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
}

static uintptr_t eval_binary_operation(simgrid::dwarf::ExpressionContext const& state, uint8_t op, uintptr_t a,
                                       uintptr_t b)
{
  std::array<Dwarf_Op, 15> ops;
  ops[0].atom = DW_OP_const8u;
  ops[0].number = a;
  ops[1].atom = DW_OP_const8u;
  ops[1].number = b;
  ops[2].atom = op;

  simgrid::dwarf::ExpressionStack stack;
  try {
    simgrid::dwarf::execute(ops.data(), 3, state, stack);
  } catch (const simgrid::dwarf::evaluation_error&) {
    fprintf(stderr,"Expression evaluation error");
  }

  assert(stack.size() == 1);
  return stack.top();
}

static void basic_test(simgrid::dwarf::ExpressionContext const& state)
{
  std::array<Dwarf_Op, 60> ops;

  uintptr_t a = rnd_engine();
  uintptr_t b = rnd_engine();

  simgrid::dwarf::ExpressionStack stack;

  bool caught_ex = false;
  try {
    ops[0].atom = DW_OP_drop;
    simgrid::dwarf::execute(ops.data(), 1, state, stack);
  } catch (const simgrid::dwarf::evaluation_error&) {
    caught_ex = true;
  }
  if (not caught_ex)
    fprintf(stderr, "Exception expected");

  try {
    ops[0].atom = DW_OP_lit21;
    simgrid::dwarf::execute(ops.data(), 1, state, stack);
    assert(stack.size() == 1);
    assert(stack.top() == 21);

    ops[0].atom   = DW_OP_const8u;
    ops[0].number = a;
    simgrid::dwarf::execute(ops.data(), 1, state, stack);
    assert(stack.size() == 2);
    assert(stack.top() == a);

    ops[0].atom = DW_OP_drop;
    ops[1].atom = DW_OP_drop;
    simgrid::dwarf::execute(ops.data(), 2, state, stack);
    assert(stack.empty());

    stack.clear();
    ops[0].atom   = DW_OP_lit21;
    ops[1].atom   = DW_OP_plus_uconst;
    ops[1].number = a;
    simgrid::dwarf::execute(ops.data(), 2, state, stack);
    assert(stack.size() == 1);
    assert(stack.top() == a + 21);

    stack.clear();
    ops[0].atom   = DW_OP_const8u;
    ops[0].number = a;
    ops[1].atom   = DW_OP_dup;
    ops[2].atom   = DW_OP_plus;
    simgrid::dwarf::execute(ops.data(), 3, state, stack);
    assert(stack.size() == 1);
    assert(stack.top() == a + a);

    stack.clear();
    ops[0].atom   = DW_OP_const8u;
    ops[0].number = a;
    ops[1].atom   = DW_OP_const8u;
    ops[1].number = b;
    ops[2].atom   = DW_OP_over;
    simgrid::dwarf::execute(ops.data(), 3, state, stack);
    assert(stack.size() == 3);
    assert(stack.top() == a);
    assert(stack.top(1) == b);
    assert(stack.top(2) == a);

    stack.clear();
    ops[0].atom   = DW_OP_const8u;
    ops[0].number = a;
    ops[1].atom   = DW_OP_const8u;
    ops[1].number = b;
    ops[2].atom   = DW_OP_swap;
    simgrid::dwarf::execute(ops.data(), 3, state, stack);
    assert(stack.size() == 2);
    assert(stack.top() == a);
    assert(stack.top(1) == b);
  } catch (const simgrid::dwarf::evaluation_error&) {
    fprintf(stderr,"Expression evaluation error");
  }
}

static void test_deref(simgrid::dwarf::ExpressionContext const& state)
{
  try {
    uintptr_t foo = 42;

    std::array<Dwarf_Op, 60> ops;
    ops[0].atom   = DW_OP_const8u;
    ops[0].number = (uintptr_t)&foo;
    ops[1].atom   = DW_OP_deref;

    simgrid::dwarf::ExpressionStack stack;

    simgrid::dwarf::execute(ops.data(), 2, state, stack);
    assert(stack.size() == 1);
    assert(stack.top() == foo);
  } catch (const simgrid::dwarf::evaluation_error&) {
    fprintf(stderr,"Expression evaluation error");
  }
}

int main()
{
  auto* process = new simgrid::mc::RemoteProcess(getpid());
  process->init(nullptr, nullptr, nullptr);

  simgrid::dwarf::ExpressionContext state;
  state.address_space = (simgrid::mc::AddressSpace*) process;

  basic_test(state);

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rnd_engine();
    uintptr_t b = rnd_engine();
    assert(eval_binary_operation(state, DW_OP_plus, a, b) == (a + b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rnd_engine();
    uintptr_t b = rnd_engine();
    assert(eval_binary_operation(state, DW_OP_or, a, b) == (a | b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rnd_engine();
    uintptr_t b = rnd_engine();
    assert(eval_binary_operation(state, DW_OP_and, a, b) == (a & b));
  }

  for(int i=0; i!=100; ++i) {
    uintptr_t a = rnd_engine();
    uintptr_t b = rnd_engine();
    assert(eval_binary_operation(state, DW_OP_xor, a, b) == (a ^ b));
  }

  test_deref(state);

  return 0;
}
