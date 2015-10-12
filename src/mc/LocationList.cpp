/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_dwarf.hpp"
#include <mc/ObjectInformation.hpp>
#include <mc/LocationList.hpp>

namespace simgrid {
namespace dwarf {

/** Resolve a location expression */
Location resolve(
                      simgrid::dwarf::DwarfExpression const& expression,
                      simgrid::mc::ObjectInformation* object_info,
                      unw_cursor_t * c,
                      void *frame_pointer_address,
                      simgrid::mc::AddressSpace* address_space, int process_index)
{
  simgrid::dwarf::ExpressionContext context;
  context.frame_base = frame_pointer_address;
  context.cursor = c;
  context.address_space = address_space;
  context.object_info = object_info;
  context.process_index = process_index;

  if (!expression.empty()
      && expression[0].atom >= DW_OP_reg0
      && expression[0].atom <= DW_OP_reg31) {
    int dwarf_register = expression[0].atom - DW_OP_reg0;
    xbt_assert(c,
      "Missing frame context for register operation DW_OP_reg%i",
      dwarf_register);
    return Location(dwarf_register_to_libunwind(dwarf_register));
  }

  simgrid::dwarf::ExpressionStack stack;
  simgrid::dwarf::execute(expression, context, stack);
  return Location((void*) stack.top());
}

// TODO, move this in a method of LocationList
static simgrid::dwarf::DwarfExpression const* find_expression(
    simgrid::dwarf::LocationList const& locations, unw_word_t ip)
{
  for (simgrid::dwarf::LocationListEntry const& entry : locations)
    if (entry.valid_for_ip(ip))
      return &entry.expression;
  return nullptr;
}

Location resolve(
  simgrid::dwarf::LocationList const& locations,
  simgrid::mc::ObjectInformation* object_info,
  unw_cursor_t * c,
  void *frame_pointer_address,
  simgrid::mc::AddressSpace* address_space,
  int process_index)
{
  unw_word_t ip = 0;
  if (c && unw_get_reg(c, UNW_REG_IP, &ip))
    xbt_die("Could not resolve IP");
  simgrid::dwarf::DwarfExpression const* expression =
    find_expression(locations, ip);
  if (!expression)
    xbt_die("Could not resolve location");
  return simgrid::dwarf::resolve(
          *expression, object_info, c,
          frame_pointer_address, address_space, process_index);
}

}
}