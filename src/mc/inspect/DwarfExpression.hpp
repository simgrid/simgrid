/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_DWARF_EXPRESSION_HPP
#define SIMGRID_MC_DWARF_EXPRESSION_HPP

#include <cstdint>
#include <cstdlib>

#include <stdexcept> // runtime_error
#include <utility>
#include <vector>

#include <elfutils/libdw.h>
#include <libunwind.h>

#include "src/mc/inspect/mc_dwarf.hpp"
#include "src/mc/mc_forward.hpp"

/** @file DwarfExpression.hpp
 *
 *  Evaluation of DWARF location expressions.
 */

namespace simgrid {
namespace dwarf {

/** A DWARF expression
 *
 *  DWARF defines a simple stack-based VM for evaluating expressions
 *  (such as locations of variables, etc.): a DWARF expression is
 *  just a sequence of dwarf instructions. We currently directly use
 *  `Dwarf_Op` from `dwarf.h` for dwarf instructions.
 */
using DwarfExpression = std::vector<Dwarf_Op>;

/** Context of evaluation of a DWARF expression
 *
 *  Some DWARF instructions need to read the CPU registers,
 *  the process memory, etc. All those information are gathered in
 *  the evaluation context.
 */
struct ExpressionContext {
  /** CPU state (registers) */
  unw_cursor_t* cursor                  = nullptr;
  void* frame_base                      = nullptr;
  const mc::AddressSpace* address_space = nullptr; /** Address space used to read memory */
  mc::ObjectInformation* object_info    = nullptr;
};

/** When an error happens in the execution of a DWARF expression */
class evaluation_error : public std::runtime_error {
public:
  explicit evaluation_error(const char* what) : std::runtime_error(what) {}
};

/** A stack for evaluating a DWARF expression
 *
 *  DWARF expressions work by manipulating a stack of integer values.
 */
class ExpressionStack {
public:
  using value_type                  = std::uintptr_t;
  static const std::size_t max_size = 64;

private:
  // Values of the stack (the top is stack_[size_ - 1]):
  uintptr_t stack_[max_size]{0};
  size_t size_ = 0;

public:
  // Access:
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  void clear() { size_ = 0; }
  uintptr_t& operator[](int i) { return stack_[i]; }
  uintptr_t const& operator[](int i) const { return stack_[i]; }

  /** Top of the stack */
  value_type& top()
  {
    if (size_ == 0)
      throw evaluation_error("Empty stack");
    return stack_[size_ - 1];
  }

  /** Access the i-th element from the top of the stack */
  value_type& top(unsigned i)
  {
    if (size_ < i)
      throw evaluation_error("Invalid element");
    return stack_[size_ - 1 - i];
  }

  /** Push a value on the top of the stack */
  void push(value_type value)
  {
    if (size_ == max_size)
      throw evaluation_error("DWARF stack overflow");
    stack_[size_++] = value;
  }

  /* Pop a value from the top of the stack */
  value_type pop()
  {
    if (size_ == 0)
      throw evaluation_error("DWARF stack underflow");
    return stack_[--size_];
  }

  // These are DWARF operations (DW_OP_foo):

  /* Push a copy of the top-value (DW_OP_dup) */
  void dup() { push(top()); }

  /* Swap the two top-most values */
  void swap() { std::swap(top(), top(1)); }
};

/** Executes a DWARF expression
 *
 *  @param ops     DWARF expression instructions
 *  @param n       number of instructions
 *  @param context evaluation context (registers, memory, etc.)
 *  @param stack   DWARf stack where the operations are executed
 */
void execute(const Dwarf_Op* ops, std::size_t n, ExpressionContext const& context, ExpressionStack& stack);

/** Executes/evaluates a DWARF expression
 *
 *  @param expression DWARF expression to execute
 *  @param context    evaluation context (registers, memory, etc.)
 *  @param stack      DWARf stack where the operations are executed
 */
inline void execute(simgrid::dwarf::DwarfExpression const& expression, ExpressionContext const& context,
                    ExpressionStack& stack)
{
  execute(expression.data(), expression.size(), context, stack);
}

} // namespace dwarf
} // namespace simgrid

#endif
