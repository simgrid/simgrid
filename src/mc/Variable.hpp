/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_VARIABLE_HPP
#define SIMGRID_MC_VARIABLE_HPP

#include <string>

#include "mc_forward.h"
#include "mc_location.h"

namespace simgrid {
namespace mc {

class Variable {
public:
  Variable();

  unsigned dwarf_offset; /* Global offset of the field. */
  int global;
  std::string name;
  unsigned type_id;
  simgrid::mc::Type* type;

  // Use either of:
  simgrid::mc::LocationList location_list;
  void* address;

  size_t start_scope;
  simgrid::mc::ObjectInformation* object_info;
};

}
}

#endif
