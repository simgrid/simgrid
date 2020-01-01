/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/inspect/Type.hpp"
#include "src/mc/inspect/mc_dwarf.hpp"
#include "src/mc/mc_private.hpp"

namespace simgrid {
namespace dwarf {

/** Resolve snapshot in the process address space
 *
 * @param object   Process address of the struct/class
 * @param type     Type of the struct/class
 * @param member   Member description
 * @param snapshot Snapshot (or nullptr)
 * @return Process address of the given member of the 'object' struct/class
 */
void* resolve_member(const void* base, const simgrid::mc::Type* /*type*/, const simgrid::mc::Member* member,
                     const simgrid::mc::AddressSpace* address_space)
{
  ExpressionContext state;
  state.frame_base    = nullptr;
  state.cursor        = nullptr;
  state.address_space = address_space;

  ExpressionStack stack;
  stack.push((ExpressionStack::value_type)base);
  simgrid::dwarf::execute(member->location_expression, state, stack);
  return (void*)stack.top();
}

} // namespace dwarf
} // namespace simgrid
