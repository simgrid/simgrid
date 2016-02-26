/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_VARIABLE_HPP
#define SIMGRID_MC_VARIABLE_HPP

#include <string>

#include <xbt/base.h>

#include "src/mc/mc_forward.h"
#include "src/mc/LocationList.hpp"

namespace simgrid {
namespace mc {

/** A variable (global or local) in the model-checked program */
class Variable {
public:
  Variable() {}
  unsigned dwarf_offset = 0; /* Global offset of the field. */
  int global = 0;
  std::string name;
  unsigned type_id = 0;
  simgrid::mc::Type* type = nullptr;
  // Use either of:
  simgrid::dwarf::LocationList location_list;
  void* address = nullptr;
  size_t start_scope = 0;
  simgrid::mc::ObjectInformation* object_info = nullptr;
};

}
}

#endif
