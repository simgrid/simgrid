/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

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

#include "src/mc/mc_forward.hpp"
#include "src/mc/AddressSpace.hpp"

/** @file DwarfExession.hpp
 *
 *  Evaluation of DWARF location expressions
 */

namespace simgrid {
namespace dwarf {

/** A DWARF expression
 *
 *  DWARF defines a simple stack-based VM for evaluating expressions
 *  (such as locations of variables, etc.): A DWARF expressions is
 *  just a sequence of dwarf instructions. We currently directly use
 *  `Dwarf_Op` from `dwarf.h` for dwarf instructions.
 */
typedef std::vector<Dwarf_Op> DwarfExpression;

/** Context of evaluation of a DWARF expression
 *
 *  Some DWARF instructions need to read the CPU registers,
 *  the process memory, etc. All those informations are gathered in
 *  the evaluation context.
 */
struct ExpressionContext {
  ExpressionContext() :
    cursor(nullptr), frame_base(nullptr), address_space(nullptr),
    object_info(nullptr), process_index(simgrid::mc::ProcessIndexMissing) {}
  /** CPU state (registers) */
  unw_cursor_t* cursor;
  void* frame_base;
  /** Address space used to read memory */
  simgrid::mc::AddressSpace* address_space;
  simgrid::mc::ObjectInformation* object_info;
  int process_index;
};

/** When an error happens in the execution of a DWARF expression */
class evaluation_error : std::runtime_error {
public:
  evaluation_error(const char* what): std::runtime_error(what) {}
  ~evaluation_error() noexcept(true);
};

/** A stack for evaluating a DWARF expression
 *
 *  DWARF expressions work by manipulating a stack of integer values.
 */
class ExpressionStack {
public:
  typedef std::uintptr_t value_type;
  static const std::size_t max_size = 64;
private:
  uintptr_t stack_[max_size];
  size_t size_;
public:
  ExpressionStack() : size_(0) {}

  // Access:
  std::size_t size() const { return size_; }
  bool empty()       const { return size_ == 0; }
  void clear()             { size_ = 0; }
  uintptr_t&       operator[](int i)       { return stack_[i]; }
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
      throw evaluation_error("Dwarf stack overflow");
    stack_[size_++] = value;
  }

  /* Pop a value from the top of the stack */
  value_type pop()
  {
    if (size_ == 0)
      throw evaluation_error("Stack underflow");
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
void execute(const Dwarf_Op* ops, std::size_t n,
  ExpressionContext const& context, ExpressionStack& stack);

/** Executes/evaluates a DWARF expression
 *
 *  @param expression DWARF expression to execute
 *  @param context    evaluation context (registers, memory, etc.)
 *  @param stack      DWARf stack where the operations are executed
 */
inline
void execute(simgrid::dwarf::DwarfExpression const& expression,
  ExpressionContext const& context, ExpressionStack& stack)
{
  execute(expression.data(), expression.size(), context, stack);
}

}
}

#endif
