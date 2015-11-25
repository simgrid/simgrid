/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_LOCATION_H
#define SIMGRID_MC_OBJECT_LOCATION_H

#include <stdint.h>

#include <vector>

#include <libunwind.h>
#include <dwarf.h>
#include <elfutils/libdw.h>

#include "simgrid_config.h"
#include "src/mc/mc_base.h"
#include "src/mc/mc_forward.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/DwarfExpression.hpp"

namespace simgrid {
namespace dwarf {

/** \brief A DWARF expression with optional validity contraints */
class LocationListEntry {
public:
  simgrid::dwarf::DwarfExpression expression;
  void* lowpc, *highpc;

  LocationListEntry() : lowpc(nullptr), highpc(nullptr) {}

  bool always_valid() const
  {
    return this->lowpc == nullptr && this->highpc == nullptr;
  }
  bool valid_for_ip(unw_word_t ip) const
  {
    return always_valid() || (
      ip >= (unw_word_t) this->lowpc &&
      ip <  (unw_word_t) this->highpc);
  }
};

typedef std::vector<LocationListEntry> LocationList;

/** Location of some variable in memory
 *
 *  The variable is located either in memory of a register.
 */
class Location {
private:
  void* memory_;
  int register_id_;
public:
  Location(void* x) :memory_(x) {}
  Location(int register_id) :
    memory_(nullptr), register_id_(register_id) {}
  // Type of location:
  bool in_register() const { return memory_ == nullptr; }
  bool in_memory()   const { return memory_ != nullptr; }

  // Get the location:
  void* address()    const { return memory_; }
  int register_id()  const { return register_id_;     }
};

XBT_PRIVATE
Location resolve(
  simgrid::dwarf::DwarfExpression const& expression,
  simgrid::mc::ObjectInformation* object_info, unw_cursor_t* c,
  void* frame_pointer_address, simgrid::mc::AddressSpace* address_space,
  int process_index);

Location resolve(
  simgrid::dwarf::LocationList const& locations,
  simgrid::mc::ObjectInformation* object_info,
  unw_cursor_t * c,
  void *frame_pointer_address,
  simgrid::mc::AddressSpace* address_space,
  int process_index);

XBT_PRIVATE
simgrid::dwarf::LocationList location_list(
  simgrid::mc::ObjectInformation& info,
  Dwarf_Attribute& attr);

}
}

#endif
