/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TYPE_HPP
#define SIMGRID_MC_TYPE_HPP

#include <vector>
#include <string>

#include <xbt/base.h>

#include "mc_forward.h"
#include "mc_location.h"

namespace simgrid {
namespace mc {

/** Represent a member of  a structure (or inheritance) */
class Member {
public:
  Member() : inheritance(false), byte_size(0), type_id(0) {}

  bool inheritance;
  std::string name;
  simgrid::mc::DwarfExpression location_expression;
  std::size_t byte_size; // Do we really need this?
  unsigned type_id;
  simgrid::mc::Type* type;

  bool has_offset_location() const
  {
    return location_expression.size() == 1 &&
      location_expression[0].atom == DW_OP_plus_uconst;
  }

  // TODO, check if this shortcut is really necessary
  int offset() const
  {
    xbt_assert(this->has_offset_location());
    return this->location_expression[0].number;
  }

  void offset(int new_offset)
  {
    Dwarf_Op op;
    op.atom = DW_OP_plus_uconst;
    op.number = new_offset;
    this->location_expression = { op };
  }
};

/** Represents a type in the program
 *
 *  It is currently used to represent members of structs and unions as well.
 */
class Type {
public:
  Type();
  Type(Type const& type) = default;
  Type& operator=(Type const&) = default;
  Type(Type&& type) = default;
  Type& operator=(Type&&) = default;

  /** The DWARF TAG of the type (e.g. DW_TAG_array_type) */
  int type;
  unsigned id; /* Offset in the section (in hexadecimal form) */
  std::string name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  unsigned type_id; /* DW_AT_type id */
  std::vector<Member> members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
  int is_pointer_type;

  simgrid::mc::Type* subtype; // DW_AT_type
  simgrid::mc::Type* full_type; // The same (but more complete) type
};

inline
Type::Type()
{
  this->type = 0;
  this->id = 0;
  this->byte_size = 0;
  this->element_count = 0;
  this->is_pointer_type = 0;
  this->type_id = 0;
  this->subtype = nullptr;
  this->full_type = nullptr;
}

}
}

#endif
