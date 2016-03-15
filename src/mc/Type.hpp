/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TYPE_HPP
#define SIMGRID_MC_TYPE_HPP

#include <cstddef>

#include <vector>
#include <string>

#include <xbt/base.h>
#include <xbt/asserts.h>

#include <dwarf.h>

#include "src/mc/mc_forward.hpp"
#include "src/mc/LocationList.hpp"

namespace simgrid {
namespace mc {

/** A member of a structure, union
 *
 *  Inheritance is seen as a special member as well.
 */
class Member {
public:
  Member() {}

  /** Whether this member represent some inherited part of the object */
  bool inheritance = false;

  /** Name of the member (if any) */
  std::string name;

  /** DWARF location expression for locating the location of the member */
  simgrid::dwarf::DwarfExpression location_expression;

  std::size_t byte_size = 0; // Do we really need this?

  unsigned type_id = 0;
  simgrid::mc::Type* type = nullptr;

  /** Whether the member is at a fixed offset from the base address */
  bool has_offset_location() const
  {
    // Recognize the expression `DW_OP_plus_uconst(offset)`:
    return location_expression.size() == 1 &&
      location_expression[0].atom == DW_OP_plus_uconst;
  }

  /** Get the offset of the member
  *
  *  This is only valid is the member is at a fixed offset from the base.
  *  This is often the case (for C types, C++ type without virtual
  *  inheritance).
  *
  *  If the location is more complex, the location expression has
  *  to be evaluated (which might need accessing the memory).
  */
  int offset() const
  {
    xbt_assert(this->has_offset_location());
    return this->location_expression[0].number;
  }

  /** Set the location of the member as a fixed offset */
  void offset(int new_offset)
  {
    // Set the expression to be `DW_OP_plus_uconst(offset)`:
    Dwarf_Op op;
    op.atom = DW_OP_plus_uconst;
    op.number = new_offset;
    this->location_expression = { op };
  }

};

/** A type in the model-checked program */
class Type {
public:
  Type() {}

  /** The DWARF TAG of the type (e.g. DW_TAG_array_type) */
  int type = 0;
  unsigned id = 0; /* Offset in the section (in hexadecimal form) */
  std::string name; /* Name of the type */
  int byte_size = 0; /* Size in bytes */
  int element_count = 0; /* Number of elements for array type */
  unsigned type_id = 0; /* DW_AT_type id */
  std::vector<Member> members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/

  simgrid::mc::Type* subtype = nullptr; // DW_AT_type
  simgrid::mc::Type* full_type = nullptr; // The same (but more complete) type
};

}
}

#endif
