  /* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** file
 *  Debug information for the MC.
 */

#ifndef MC_OBJECT_INFO_H
#define MC_OBJECT_INFO_H

#include <stdint.h>

#include <simgrid_config.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>

#include "mc_forward.h"
#include "mc_location.h"
#include "mc_process.h"

SG_BEGIN_DECL();

// ***** Type

typedef int e_dw_type_type;

struct s_dw_type {
  e_dw_type_type type;
  Dwarf_Off id; /* Offset in the section (in hexadecimal form) */
  char *name; /* Name of the type */
  int byte_size; /* Size in bytes */
  int element_count; /* Number of elements for array type */
  char *dw_type_id; /* DW_AT_type id */
  xbt_dynar_t members; /* if DW_TAG_structure_type, DW_TAG_class_type, DW_TAG_union_type*/
  int is_pointer_type;

  // Location (for members) is either of:
  struct s_mc_expression location;
  int offset;

  dw_type_t subtype; // DW_AT_type
  dw_type_t full_type; // The same (but more complete) type
};

void dw_type_free(dw_type_t t);
void dw_variable_free(dw_variable_t v);
void dw_variable_free_voidp(void *t);

// ***** Object info

enum mc_object_info_flags {
  MC_OBJECT_INFO_NONE = 0,
  MC_OBJECT_INFO_EXECUTABLE = 1
};

struct s_mc_object_info {
  enum mc_object_info_flags flags;
  char* file_name;
  char *start_exec, *end_exec; // Executable segment
  char *start_rw, *end_rw; // Read-write segment
  char *start_ro, *end_ro; // read-only segment
  xbt_dict_t subprograms; // xbt_dict_t<origin as hexadecimal string, dw_frame_t>
  xbt_dynar_t global_variables; // xbt_dynar_t<dw_variable_t>
  xbt_dict_t types; // xbt_dict_t<origin as hexadecimal string, dw_type_t>
  xbt_dict_t full_types_by_name; // xbt_dict_t<name, dw_type_t> (full defined type only)

  // Here we sort the minimal information for an efficient (and cache-efficient)
  // lookup of a function given an instruction pointer.
  // The entries are sorted by low_pc and a binary search can be used to look them up.
  xbt_dynar_t functions_index;
};

static inline __attribute__ ((always_inline))
bool MC_object_info_executable(mc_object_info_t info)
{
  return info->flags & MC_OBJECT_INFO_EXECUTABLE;
}

/** Find the DWARF offset for this ELF object
 *
 *  An offset is applied to address found in DWARF:
 *
 *  <ul>
 *    <li>for an executable obejct, addresses are virtual address
 *        (there is no offset) i.e. \f$\text{virtual address} = \{dwarf address}\f$;</li>
 *    <li>for a shared object, the addreses are offset from the begining
 *        of the shared object (the base address of the mapped shared
 *        object must be used as offset
 *        i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.</li>
 *
 */
void* MC_object_base_address(mc_object_info_t info);

mc_object_info_t MC_new_object_info(void);
mc_object_info_t MC_find_object_info(memory_map_t maps, const char* name, int executable);
void MC_free_object_info(mc_object_info_t* p);

dw_frame_t MC_file_object_info_find_function(mc_object_info_t info, void *ip);
dw_variable_t MC_file_object_info_find_variable_by_name(mc_object_info_t info, const char* name);

void MC_post_process_object_info(mc_process_t process, mc_object_info_t info);

void MC_dwarf_get_variables(mc_object_info_t info);
void MC_dwarf_get_variables_libdw(mc_object_info_t info);
const char* MC_dwarf_attrname(int attr);
const char* MC_dwarf_tagname(int tag);

// Not used:
char* get_type_description(mc_object_info_t info, char *type_name);

void* mc_member_resolve(const void* base, dw_type_t type, dw_type_t member, mc_snapshot_t snapshot, int process_index);

struct s_dw_variable{
  Dwarf_Off dwarf_offset; /* Global offset of the field. */
  int global;
  char *name;
  char *type_origin;
  dw_type_t type;

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

void mc_frame_free(dw_frame_t freme);

SG_END_DECL()

#endif
