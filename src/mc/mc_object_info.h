/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** file
 *  Debug information for the MC.
 */

#ifndef SIMGRID_MC_OBJECT_INFO_H
#define SIMGRID_MC_OBJECT_INFO_H

#include <stdint.h>

#include <string>

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
  ~Type();
  Type(Type const& type) = delete;
  Type& operator=(Type const&) = delete;

  e_mc_type_type type;
  Dwarf_Off id; /* Offset in the section (in hexadecimal form) */
  std::string name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  std::string dw_type_id; /* DW_AT_type id */
  xbt_dynar_t members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
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

class ObjectInformation {
public:
  ObjectInformation();
  ~ObjectInformation();
  ObjectInformation(ObjectInformation const&) = delete;
  ObjectInformation& operator=(ObjectInformation const&) = delete;

  mc_object_info_flags flags;
  char* file_name;
  const void* start;
  const void *end;
  char *start_exec;
  char *end_exec; // Executable segment
  char *start_rw;
  char *end_rw; // Read-write segment
  char *start_ro;
  char *end_ro; // read-only segment
  xbt_dict_t subprograms; // xbt_dict_t<origin as hexadecimal string, dw_frame_t>
  xbt_dynar_t global_variables; // xbt_dynar_t<dw_variable_t>
  xbt_dict_t types; // xbt_dict_t<origin as hexadecimal string, mc_type_t>
  xbt_dict_t full_types_by_name; // xbt_dict_t<name, mc_type_t> (full defined type only)

  // Here we sort the minimal information for an efficient (and cache-efficient)
  // lookup of a function given an instruction pointer.
  // The entries are sorted by low_pc and a binary search can be used to look them up.
  xbt_dynar_t functions_index;

  bool executable() const
  {
    return this->flags & MC_OBJECT_INFO_EXECUTABLE;
  }

  bool privatized() const
  {
    return this->executable() && smpi_privatize_global_variables;
  }

  void* base_address() const;

  dw_frame_t find_function(const void *ip) const;
  dw_variable_t find_variable(const char* name) const;

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

XBT_INTERNAL void* mc_member_resolve(const void* base, mc_type_t type, mc_type_t member, mc_address_space_t snapshot, int process_index);

struct s_dw_variable{
  Dwarf_Off dwarf_offset; /* Global offset of the field. */
  int global;
  char *name;
  char *type_origin;
  mc_type_t type;

  // Use either of:
  s_mc_location_list_t locations;
  void* address;

  size_t start_scope;
  mc_object_info_t object_info;

};

struct s_dw_frame{
  int tag;
  char *name;
  void *low_pc;
  void *high_pc;
  s_mc_location_list_t frame_base;
  xbt_dynar_t /* <dw_variable_t> */ variables; /* Cannot use dict, there may be several variables with the same name (in different lexical blocks)*/
  unsigned long int id; /* DWARF offset of the subprogram */
  xbt_dynar_t /* <dw_frame_t> */ scopes;
  Dwarf_Off abstract_origin_id;
  mc_object_info_t object_info;
};

struct s_mc_function_index_item {
  void* low_pc, *high_pc;
  dw_frame_t function;
};

XBT_INTERNAL void mc_frame_free(dw_frame_t freme);

#endif
