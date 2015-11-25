/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/misc.h>

#include "src/mc/mc_object_info.h"
#include "src/mc/mc_private.h"
#include "src/mc/Type.hpp"

namespace simgrid {
namespace dwarf {

/** Resolve snapshot in the process address space
 *
 * @param object   Process address of the struct/class
 * @param type     Type of the struct/class
 * @param member   Member description
 * @param snapshot Snapshot (or NULL)
 * @return Process address of the given member of the 'object' struct/class
 */
void *resolve_member(
    const void *base, simgrid::mc::Type* type, simgrid::mc::Member* member,
    simgrid::mc::AddressSpace* address_space, int process_index)
{
  // TODO, get rid of this?
  if (!member->has_offset_location())
    return ((char *) base) + member->offset();

  ExpressionContext state;
  state.frame_base = NULL;
  state.cursor = NULL;
  state.address_space = address_space;
  state.process_index = process_index;

  ExpressionStack stack;
  stack.push((ExpressionStack::value_type) base);
  simgrid::dwarf::execute(member->location_expression, state, stack);
  return (void*) stack.top();
}

}
}
