/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_FRAME_HPP
#define SIMGRID_MC_FRAME_HPP

#include <string>
#include <cstdint>

#include <xbt/base.h>
#include <xbt/range.hpp>

#include "src/mc/mc_forward.hpp"
#include "src/mc/LocationList.hpp"
#include "src/mc/Variable.hpp"

namespace simgrid {
namespace mc {

/** Debug information about a given function or scope within a function */
class Frame {
public:
  Frame();

  /** Kind of scope (DW_TAG_subprogram, DW_TAG_inlined_subroutine, etc.) */
  int tag = DW_TAG_invalid;

  /** Name of the function (if it is a function) */
  std::string name;

  /** Range of instruction addresses for which this scope is valid */
  simgrid::xbt::Range<std::uint64_t> range;

  simgrid::dwarf::LocationList frame_base_location;

  /** List of the variables (sorted by name) */
  std::vector<Variable> variables;

  /* Unique identifier for this scope (in the object_info)
   *
   * This is the global DWARF offset of the DIE. */
  unsigned long int id = 0;

  std::vector<Frame> scopes;

  /** Value of `DW_AT_abstract_origin`
   *
   *  For inlined subprograms, this is the ID of the
   *  parent function.
   */
  unsigned long int abstract_origin_id = 0;

  simgrid::mc::ObjectInformation* object_info = nullptr;

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
