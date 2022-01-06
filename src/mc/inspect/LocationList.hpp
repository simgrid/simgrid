/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_LOCATION_H
#define SIMGRID_MC_OBJECT_LOCATION_H

#include "xbt/base.h"
#include "xbt/range.hpp"

#include "src/mc/inspect/DwarfExpression.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_forward.hpp"

#include <cstdint>
#include <vector>

namespace simgrid {
namespace dwarf {

/** A DWARF expression with optional validity constraints */
class LocationListEntry {
public:
  using range_type = simgrid::xbt::Range<std::uint64_t>;

private:
  DwarfExpression expression_;
  // By default, the expression is always valid:
  range_type range_ = {0, UINT64_MAX};

public:
  LocationListEntry() = default;
  LocationListEntry(DwarfExpression expression, range_type range) : expression_(std::move(expression)), range_(range) {}
  explicit LocationListEntry(DwarfExpression expression) : expression_(std::move(expression)), range_({0, UINT64_MAX})
  {
  }

  DwarfExpression& expression() { return expression_; }
  DwarfExpression const& expression() const { return expression_; }
  bool valid_for_ip(unw_word_t ip) const { return range_.contain(ip); }
};

using LocationList = std::vector<LocationListEntry>;

/** Location of some variable in memory
 *
 *  The variable is located either in memory of a register.
 */
class Location {
private:
  void* memory_    = nullptr;
  int register_id_ = 0;

public:
  explicit Location(void* x) : memory_(x) {}
  explicit Location(int register_id) : register_id_(register_id) {}
  // Type of location:
  bool in_register() const { return memory_ == nullptr; }
  bool in_memory() const { return memory_ != nullptr; }

  // Get the location:
  void* address() const { return memory_; }
  int register_id() const { return register_id_; }
};

XBT_PRIVATE
Location resolve(simgrid::dwarf::DwarfExpression const& expression, simgrid::mc::ObjectInformation* object_info,
                 unw_cursor_t* c, void* frame_pointer_address, const simgrid::mc::AddressSpace* address_space);

Location resolve(simgrid::dwarf::LocationList const& locations, simgrid::mc::ObjectInformation* object_info,
                 unw_cursor_t* c, void* frame_pointer_address, const simgrid::mc::AddressSpace* address_space);

XBT_PRIVATE
simgrid::dwarf::LocationList location_list(const simgrid::mc::ObjectInformation& info, Dwarf_Attribute& attr);

} // namespace dwarf
} // namespace simgrid

#endif
