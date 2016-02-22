/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_FRAME_HPP
#define SIMGRID_MC_FRAME_HPP

#include <string>

#include <xbt/base.h>
#include <xbt/range.hpp>

#include "src/mc/mc_forward.h"
#include "src/mc/LocationList.hpp"
#include "src/mc/Variable.hpp"
#include "src/mc/Frame.hpp"

namespace simgrid {
namespace mc {

/** Debug information about a given function or scope within a function */
class Frame {
public:
  Frame();

  int tag;
  std::string name;
  /** Range of instruction addresses for which this scope is valid */
  simgrid::xbt::range<std::uint64_t> range;
  simgrid::dwarf::LocationList frame_base_location;
  std::vector<Variable> variables;
  unsigned long int id; /* DWARF offset of the subprogram */
  std::vector<Frame> scopes;
  unsigned long int abstract_origin_id;
  simgrid::mc::ObjectInformation* object_info;

  void* frame_base(unw_cursor_t& unw_cursor) const;
  void remove_variable(char* name);
};

inline
Frame::Frame()
{
  this->tag = 0;
  this->range = {0, 0};
  this->id = 0;
  this->abstract_origin_id = 0;
  this->object_info = nullptr;
}

}
}

#endif
