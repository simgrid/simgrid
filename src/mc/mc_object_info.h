/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** file
 *  Debug information for the MC.
 */

#ifndef SIMGRID_MC_OBJECT_INFO_H
#define SIMGRID_MC_OBJECT_INFO_H

#include <cstdint>

#include <string>
#include <vector>
#include <unordered_map>

#include <simgrid_config.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>

#include <elfutils/libdw.h>

#include "mc_forward.hpp"
#include "mc_location.h"
#include "mc_process.h"
#include "../smpi/private.h"

// ***** Type

namespace simgrid {
namespace mc {

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
  Dwarf_Off id; /* Offset in the section (in hexadecimal form) */
  std::string name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  std::uint64_t type_id; /* DW_AT_type id */
  std::vector<Type> members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
  int is_pointer_type;

  // Location (for members) is either of:
  simgrid::mc::DwarfExpression location_expression;

  simgrid::mc::Type* subtype; // DW_AT_type
  simgrid::mc::Type* full_type; // The same (but more complete) type

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

}
}

// ***** Object info

namespace simgrid {
namespace mc {

class Variable {
public:
  Variable();

  Dwarf_Off dwarf_offset; /* Global offset of the field. */
  int global;
  std::string name;
  std::uint64_t type_id;
  simgrid::mc::Type* type;

  // Use either of:
  simgrid::mc::LocationList location_list;
  void* address;

  size_t start_scope;
  simgrid::mc::ObjectInformation* object_info;
};

class Frame {
public:
  Frame();

  int tag;
  std::string name;
  void *low_pc;
  void *high_pc;
  simgrid::mc::LocationList frame_base;
  std::vector<Variable> variables;
  unsigned long int id; /* DWARF offset of the subprogram */
  std::vector<Frame> scopes;
  Dwarf_Off abstract_origin_id;
  simgrid::mc::ObjectInformation* object_info;
};

/** An entry in the functions index
 *
 *  See the code of ObjectInformation::find_function.
 */
struct FunctionIndexEntry {
  void* low_pc;
  simgrid::mc::Frame* function;
};

/** Information about an (ELF) executable/sharedobject
 *
 *  This contain sall the information we have at runtime about an
 *  executable/shared object in the target (modelchecked) process:
 *  - where it is located in the virtual address space;
 *  - where are located it's different memory mapping in the the
 *    virtual address space ;
 *  - all the debugging (DWARF) information,
 *    - location of the functions,
 *    - types
 *  - etc.
 *
 *  It is not copyable because we are taking pointers to Types/Frames.
 *  We'd have to update/rebuild some data structures in order to copy
 *  successfully.
 */

class ObjectInformation {
public:
  ObjectInformation();

  // Not copyable:
  ObjectInformation(ObjectInformation const&) = delete;
  ObjectInformation& operator=(ObjectInformation const&) = delete;

  // Flag:
  static const int Executable = 1;

  /** Bitfield of flags */
  int flags;
  std::string file_name;
  const void* start;
  const void *end;
  char *start_exec;
  char *end_exec; // Executable segment
  char *start_rw;
  char *end_rw; // Read-write segment
  char *start_ro;
  char *end_ro; // read-only segment
  std::unordered_map<std::uint64_t, simgrid::mc::Frame> subprograms;
  // TODO, remove the mutable (to remove it we'll have to add a lot of const everywhere)
  mutable std::vector<simgrid::mc::Variable> global_variables;
  std::unordered_map<std::uint64_t, simgrid::mc::Type> types;
  std::unordered_map<std::string, simgrid::mc::Type*> full_types_by_name;

  /** Index of functions by IP
   *
   * The entries are sorted by low_pc and a binary search can be used to look
   * them up. Should we used a binary tree instead?
   */
  std::vector<FunctionIndexEntry> functions_index;

  bool executable() const
  {
    return this->flags & simgrid::mc::ObjectInformation::Executable;
  }

  bool privatized() const
  {
    return this->executable() && smpi_privatize_global_variables;
  }

  void* base_address() const;

  simgrid::mc::Frame* find_function(const void *ip) const;
  simgrid::mc::Variable* find_variable(const char* name) const;

};

}
}


XBT_INTERNAL std::shared_ptr<simgrid::mc::ObjectInformation> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char* name, int executable);
XBT_INTERNAL void MC_post_process_object_info(simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info);

XBT_INTERNAL void MC_dwarf_get_variables(simgrid::mc::ObjectInformation* info);
XBT_INTERNAL void MC_dwarf_get_variables_libdw(simgrid::mc::ObjectInformation* info);
XBT_INTERNAL const char* MC_dwarf_attrname(int attr);
XBT_INTERNAL const char* MC_dwarf_tagname(int tag);

XBT_INTERNAL void* mc_member_resolve(
  const void* base, simgrid::mc::Type* type, simgrid::mc::Type* member,
  simgrid::mc::AddressSpace* snapshot, int process_index);

#endif
