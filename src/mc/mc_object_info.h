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

#include "mc_forward.h"
#include "mc_location.h"
#include "mc_process.h"
#include "../smpi/private.h"

// ***** Type

typedef int e_mc_type_type;

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

  e_mc_type_type type;
  Dwarf_Off id; /* Offset in the section (in hexadecimal form) */
  std::string name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  std::uint64_t type_id; /* DW_AT_type id */
  std::vector<Type> members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
  int is_pointer_type;

  // Location (for members) is either of:
  simgrid::mc::DwarfExpression location_expression;

  mc_type_t subtype; // DW_AT_type
  mc_type_t full_type; // The same (but more complete) type

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

/** Bit field of options */
typedef int mc_object_info_flags;
#define MC_OBJECT_INFO_NONE 0
#define MC_OBJECT_INFO_EXECUTABLE 1

namespace simgrid {
namespace mc {

class Variable {
public:
  Variable();

  Dwarf_Off dwarf_offset; /* Global offset of the field. */
  int global;
  std::string name;
  std::uint64_t type_id;
  mc_type_t type;

  // Use either of:
  simgrid::mc::LocationList location_list;
  void* address;

  size_t start_scope;
  mc_object_info_t object_info;
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
  mc_object_info_t object_info;
};

/** An entry in the functions index
 *
 *  See the code of ObjectInformation::find_function.
 */
struct FunctionIndexEntry {
  void* low_pc;
  mc_frame_t function;
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

  mc_object_info_flags flags;
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
    return this->flags & MC_OBJECT_INFO_EXECUTABLE;
  }

  bool privatized() const
  {
    return this->executable() && smpi_privatize_global_variables;
  }

  void* base_address() const;

  mc_frame_t find_function(const void *ip) const;
  // TODO, should be simgrid::mc::Variable*
  simgrid::mc::Variable* find_variable(const char* name) const;

};

}
}


XBT_INTERNAL std::shared_ptr<s_mc_object_info_t> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char* name, int executable);
XBT_INTERNAL void MC_post_process_object_info(mc_process_t process, mc_object_info_t info);

XBT_INTERNAL void MC_dwarf_get_variables(mc_object_info_t info);
XBT_INTERNAL void MC_dwarf_get_variables_libdw(mc_object_info_t info);
XBT_INTERNAL const char* MC_dwarf_attrname(int attr);
XBT_INTERNAL const char* MC_dwarf_tagname(int tag);

XBT_INTERNAL void* mc_member_resolve(
  const void* base, mc_type_t type, mc_type_t member,
  mc_address_space_t snapshot, int process_index);

#endif
