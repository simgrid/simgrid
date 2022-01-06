/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/inspect/LocationList.hpp"
#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/mc_dwarf.hpp"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <cstddef>
#include <cstdint>
#include <libunwind.h>
#include <utility>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(mc_dwarf);

namespace simgrid {
namespace dwarf {

/** Resolve a location expression */
Location resolve(simgrid::dwarf::DwarfExpression const& expression, simgrid::mc::ObjectInformation* object_info,
                 unw_cursor_t* c, void* frame_pointer_address, const simgrid::mc::AddressSpace* address_space)
{
  simgrid::dwarf::ExpressionContext context;
  context.frame_base    = frame_pointer_address;
  context.cursor        = c;
  context.address_space = address_space;
  context.object_info   = object_info;

  if (not expression.empty() && expression[0].atom >= DW_OP_reg0 && expression[0].atom <= DW_OP_reg31) {
    int dwarf_register = expression[0].atom - DW_OP_reg0;
    xbt_assert(c, "Missing frame context for register operation DW_OP_reg%i", dwarf_register);
    return Location(dwarf_register_to_libunwind(dwarf_register));
  }

  simgrid::dwarf::ExpressionStack stack;
  simgrid::dwarf::execute(expression, context, stack);
  return Location((void*)stack.top());
}

// TODO, move this in a method of LocationList
static simgrid::dwarf::DwarfExpression const* find_expression(simgrid::dwarf::LocationList const& locations,
                                                              unw_word_t ip)
{
  for (simgrid::dwarf::LocationListEntry const& entry : locations)
    if (entry.valid_for_ip(ip))
      return &entry.expression();
  return nullptr;
}

Location resolve(simgrid::dwarf::LocationList const& locations, simgrid::mc::ObjectInformation* object_info,
                 unw_cursor_t* c, void* frame_pointer_address, const simgrid::mc::AddressSpace* address_space)
{
  unw_word_t ip = 0;
  if (c)
    xbt_assert(unw_get_reg(c, UNW_REG_IP, &ip) == 0, "Could not resolve IP");
  simgrid::dwarf::DwarfExpression const* expression = find_expression(locations, ip);
  xbt_assert(expression != nullptr, "Could not resolve location");
  return simgrid::dwarf::resolve(*expression, object_info, c, frame_pointer_address, address_space);
}

LocationList location_list(const simgrid::mc::ObjectInformation& info, Dwarf_Attribute& attr)
{
  LocationList locations;
  std::ptrdiff_t offset = 0;
  while (true) {
    Dwarf_Addr base;
    Dwarf_Addr start;
    Dwarf_Addr end;
    Dwarf_Op* ops;
    std::size_t len;

    offset = dwarf_getlocations(&attr, offset, &base, &start, &end, &ops, &len);

    if (offset == -1)
      XBT_WARN("Error while loading location list: %s", dwarf_errmsg(-1));
    if (offset <= 0)
      break;

    auto base_address = reinterpret_cast<std::uint64_t>(info.base_address());

    LocationListEntry::range_type range;
    if (start == 0)
      // If start == 0, this is not a location list:
      range = {0, UINT64_MAX};
    else
      range = {base_address + start, base_address + end};

    locations.emplace_back(DwarfExpression(ops, ops + len), range);
  }

  return locations;
}
} // namespace dwarf
} // namespace simgrid
