/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_VARIABLE_HPP
#define SIMGRID_MC_VARIABLE_HPP

#include <string>

#include <xbt/base.h>

#include "mc_forward.h"
#include "mc/LocationList.hpp"

namespace simgrid {
namespace mc {

/** A variable (global or local) in the model-checked program */
class Variable {
public:
  Variable();

  unsigned dwarf_offset; /* Global offset of the field. */
  int global;
  std::string name;
  unsigned type_id;
  simgrid::mc::Type* type;

  // Use either of:
  simgrid::dwarf::LocationList location_list;
  void* address;

  size_t start_scope;
  simgrid::mc::ObjectInformation* object_info;
};

inline
Variable::Variable()
{
  this->dwarf_offset = 0;
  this->global = 0;
  this->type = nullptr;
  this->type_id = 0;
  this->address = nullptr;
  this->start_scope = 0;
  this->object_info = nullptr;
}

}
}

#endif
