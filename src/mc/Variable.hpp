/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_VARIABLE_HPP
#define SIMGRID_MC_VARIABLE_HPP

#include <cstddef>

#include <string>

#include "src/mc/mc_forward.hpp"
#include "src/mc/LocationList.hpp"

namespace simgrid {
namespace mc {

/** A variable (global or local) in the model-checked program */
class Variable {
public:
  Variable() {}
  std::uint32_t id = 0;
  bool global = false;
  std::string name;
  unsigned type_id = 0;
  simgrid::mc::Type* type = nullptr;

  /** Address of the variable (if it is fixed) */
  void* address = nullptr;

  /** Description of the location of the variable (if it's not fixed) */
  simgrid::dwarf::LocationList location_list;

  /** Offset of validity of the variable (DW_AT_start_scope)
   *
   *  This is an offset from the variable scope beginning. This variable
   *  is only valid starting from this offset.
   */
  std::size_t start_scope = 0;

  simgrid::mc::ObjectInformation* object_info = nullptr;
};

}
}

#endif
