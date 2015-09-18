/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#if !defined(SIMGRID_MC_DWARF_HPP)
#define SIMGRID_MC_DWARF_HPP

#include <memory>

#include <string.h>

#include <xbt/sysdep.h>

#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */
#include <dwarf.h>

#include "mc/Variable.hpp"
#include "mc/mc_memory_map.h"

/** \brief A class of DWARF tags (DW_TAG_*)
 */
typedef enum mc_tag_class {
  mc_tag_unknown,
  mc_tag_type,
  mc_tag_subprogram,
  mc_tag_variable,
  mc_tag_scope,
  mc_tag_namespace
} mc_tag_class;

static mc_tag_class MC_dwarf_tag_classify(int tag)
{
  switch (tag) {

  case DW_TAG_array_type:
  case DW_TAG_class_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_typedef:
  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
  case DW_TAG_string_type:
  case DW_TAG_structure_type:
  case DW_TAG_subroutine_type:
  case DW_TAG_union_type:
  case DW_TAG_ptr_to_member_type:
  case DW_TAG_set_type:
  case DW_TAG_subrange_type:
  case DW_TAG_base_type:
  case DW_TAG_const_type:
  case DW_TAG_file_type:
  case DW_TAG_packed_type:
  case DW_TAG_volatile_type:
  case DW_TAG_restrict_type:
  case DW_TAG_interface_type:
  case DW_TAG_unspecified_type:
  case DW_TAG_shared_type:
    return mc_tag_type;

  case DW_TAG_subprogram:
    return mc_tag_subprogram;

  case DW_TAG_variable:
  case DW_TAG_formal_parameter:
    return mc_tag_variable;

  case DW_TAG_lexical_block:
  case DW_TAG_try_block:
  case DW_TAG_catch_block:
  case DW_TAG_inlined_subroutine:
  case DW_TAG_with_stmt:
    return mc_tag_scope;

  case DW_TAG_namespace:
    return mc_tag_namespace;

  default:
    return mc_tag_unknown;

  }
}

#define MC_DW_CLASS_UNKNOWN 0
#define MC_DW_CLASS_ADDRESS 1   // Location in the address space of the program
#define MC_DW_CLASS_BLOCK 2     // Arbitrary block of bytes
#define MC_DW_CLASS_CONSTANT 3
#define MC_DW_CLASS_STRING 3    // String
#define MC_DW_CLASS_FLAG 4      // Boolean
#define MC_DW_CLASS_REFERENCE 5 // Reference to another DIE
#define MC_DW_CLASS_EXPRLOC 6   // DWARF expression/location description
#define MC_DW_CLASS_LINEPTR 7
#define MC_DW_CLASS_LOCLISTPTR 8
#define MC_DW_CLASS_MACPTR 9
#define MC_DW_CLASS_RANGELISTPTR 10

/** \brief Find the DWARF data class for a given DWARF data form
 *
 *  This mapping is defined in the DWARF spec.
 *
 *  \param form The form (values taken from the DWARF spec)
 *  \return An internal representation for the corresponding class
 * */
static int MC_dwarf_form_get_class(int form)
{
  switch (form) {
  case DW_FORM_addr:
    return MC_DW_CLASS_ADDRESS;
  case DW_FORM_block2:
  case DW_FORM_block4:
  case DW_FORM_block:
  case DW_FORM_block1:
    return MC_DW_CLASS_BLOCK;
  case DW_FORM_data1:
  case DW_FORM_data2:
  case DW_FORM_data4:
  case DW_FORM_data8:
  case DW_FORM_udata:
  case DW_FORM_sdata:
    return MC_DW_CLASS_CONSTANT;
  case DW_FORM_string:
  case DW_FORM_strp:
    return MC_DW_CLASS_STRING;
  case DW_FORM_ref_addr:
  case DW_FORM_ref1:
  case DW_FORM_ref2:
  case DW_FORM_ref4:
  case DW_FORM_ref8:
  case DW_FORM_ref_udata:
    return MC_DW_CLASS_REFERENCE;
  case DW_FORM_flag:
  case DW_FORM_flag_present:
    return MC_DW_CLASS_FLAG;
  case DW_FORM_exprloc:
    return MC_DW_CLASS_EXPRLOC;
    // TODO sec offset
    // TODO indirect
  default:
    return MC_DW_CLASS_UNKNOWN;
  }
}

/** \brief Find the default lower bound for a given language
 *
 *  The default lower bound of an array (when DW_TAG_lower_bound
 *  is missing) depends on the language of the compilation unit.
 *
 *  \param lang Language of the compilation unit (values defined in the DWARF spec)
 *  \return     Default lower bound of an array in this compilation unit
 * */
static inline
uint64_t MC_dwarf_default_lower_bound(int lang)
{
  switch (lang) {
  case DW_LANG_C:
  case DW_LANG_C89:
  case DW_LANG_C99:
  case DW_LANG_C_plus_plus:
  case DW_LANG_D:
  case DW_LANG_Java:
  case DW_LANG_ObjC:
  case DW_LANG_ObjC_plus_plus:
  case DW_LANG_Python:
  case DW_LANG_UPC:
    return 0;
  case DW_LANG_Ada83:
  case DW_LANG_Ada95:
  case DW_LANG_Fortran77:
  case DW_LANG_Fortran90:
  case DW_LANG_Fortran95:
  case DW_LANG_Modula2:
  case DW_LANG_Pascal83:
  case DW_LANG_PL1:
  case DW_LANG_Cobol74:
  case DW_LANG_Cobol85:
    return 1;
  default:
    xbt_die("No default DW_TAG_lower_bound for language %i and none given",
            lang);
    return 0;
  }
}

/** Sort the variable by name and address.
 *
 *  We could use boost::container::flat_set instead.
 */
static inline
bool MC_compare_variable(
  simgrid::mc::Variable const& a, simgrid::mc::Variable const& b)
{
  int cmp = a.name.compare(b.name);
  if (cmp < 0)
    return true;
  else if (cmp > 0)
    return false;
  else
    return a.address < b.address;
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
