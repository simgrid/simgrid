/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>

#include <stdlib.h>
#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */
#include <dwarf.h>
#include <elfutils/libdw.h>

#include <simgrid_config.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "mc_object_info.h"
#include "mc_private.h"

static void dw_variable_free(dw_variable_t v);
static void dw_variable_free_voidp(void *t);

static void MC_dwarf_register_global_variable(mc_object_info_t info, dw_variable_t variable);
static void MC_register_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);
static void MC_dwarf_register_non_global_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);
static void MC_dwarf_register_variable(mc_object_info_t info, dw_frame_t frame, dw_variable_t variable);

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dwarf, mc, "DWARF processing");

/** \brief The default DW_TAG_lower_bound for a given DW_AT_language.
 *
 *  The default for a given language is defined in the DWARF spec.
 *
 *  \param language consant as defined by the DWARf spec
 */
static uint64_t MC_dwarf_default_lower_bound(int lang);

/** \brief Computes the the element_count of a DW_TAG_enumeration_type DIE
 *
 * This is the number of elements in a given array dimension.
 *
 * A reference of the compilation unit (DW_TAG_compile_unit) is
 * needed because the default lower bound (when there is no DW_AT_lower_bound)
 * depends of the language of the compilation unit (DW_AT_language).
 *
 * \param die  DIE for the DW_TAG_enumeration_type or DW_TAG_subrange_type
 * \param unit DIE of the DW_TAG_compile_unit
 */
static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die * die,
                                                Dwarf_Die * unit);

/** \brief Computes the number of elements of a given DW_TAG_array_type.
 *
 * \param die DIE for the DW_TAG_array_type
 */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die * die, Dwarf_Die * unit);

/** \brief Process a DIE
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die * die,
                                Dwarf_Die * unit, dw_frame_t frame,
                                const char *ns);

/** \brief Process a type DIE
 */
static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die * die,
                                     Dwarf_Die * unit, dw_frame_t frame,
                                     const char *ns);

/** \brief Calls MC_dwarf_handle_die on all childrend of the given die
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die * die,
                                     Dwarf_Die * unit, dw_frame_t frame,
                                     const char *ns);

/** \brief Handle a variable (DW_TAG_variable or other)
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_variable_die(mc_object_info_t info, Dwarf_Die * die,
                                         Dwarf_Die * unit, dw_frame_t frame,
                                         const char *ns);

/** \brief Get the DW_TAG_type of the DIE
 *
 *  \param die DIE
 *  \return DW_TAG_type attribute as a new string (NULL if none)
 */
static char *MC_dwarf_at_type(Dwarf_Die * die);

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

/** \brief Get the name of the tag of a given DIE
 *
 *  \param die DIE
 *  \return name of the tag of this DIE
 */
static inline const char *MC_dwarf_die_tagname(Dwarf_Die * die)
{
  return MC_dwarf_tagname(dwarf_tag(die));
}

// ***** Attributes

/** \brief Get an attribute of a given DIE as a string
 *
 *  \param die       the DIE
 *  \param attribute attribute
 *  \return value of the given attribute of the given DIE
 */
static const char *MC_dwarf_attr_integrate_string(Dwarf_Die * die,
                                                  int attribute)
{
  Dwarf_Attribute attr;
  if (!dwarf_attr_integrate(die, attribute, &attr)) {
    return NULL;
  } else {
    return dwarf_formstring(&attr);
  }
}

/** \brief Get the linkage name of a DIE.
 *
 *  Use either DW_AT_linkage_name or DW_AT_MIPS_linkage_name.
 *  DW_AT_linkage_name is standardized since DWARF 4.
 *  Before this version of DWARF, the MIPS extensions
 *  DW_AT_MIPS_linkage_name is used (at least by GCC).
 *
 *  \param  the DIE
 *  \return linkage name of the given DIE (or NULL)
 * */
static const char *MC_dwarf_at_linkage_name(Dwarf_Die * die)
{
  const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_linkage_name);
  if (!name)
    name = MC_dwarf_attr_integrate_string(die, DW_AT_MIPS_linkage_name);
  return name;
}

static Dwarf_Off MC_dwarf_attr_dieoffset(Dwarf_Die * die, int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, attribute)) {
    dwarf_attr_integrate(die, attribute, &attr);
    Dwarf_Die subtype_die;
    if (dwarf_formref_die(&attr, &subtype_die) == NULL) {
      xbt_die("Could not find DIE");
    }
    return dwarf_dieoffset(&subtype_die);
  } else
    return 0;
}

static Dwarf_Off MC_dwarf_attr_integrate_dieoffset(Dwarf_Die * die,
                                                   int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, attribute)) {
    dwarf_attr_integrate(die, DW_AT_type, &attr);
    Dwarf_Die subtype_die;
    if (dwarf_formref_die(&attr, &subtype_die) == NULL) {
      xbt_die("Could not find DIE");
    }
    return dwarf_dieoffset(&subtype_die);
  } else
    return 0;
}

/** \brief Find the type/subtype (DW_AT_type) for a DIE
 *
 *  \param dit the DIE
 *  \return DW_AT_type reference as a global offset in hexadecimal (or NULL)
 */
static char *MC_dwarf_at_type(Dwarf_Die * die)
{
  Dwarf_Off offset = MC_dwarf_attr_integrate_dieoffset(die, DW_AT_type);
  return offset == 0 ? NULL : bprintf("%" PRIx64, offset);
}

static uint64_t MC_dwarf_attr_integrate_addr(Dwarf_Die * die, int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate(die, attribute, &attr) == NULL)
    return 0;
  Dwarf_Addr value;
  if (dwarf_formaddr(&attr, &value) == 0)
    return (uint64_t) value;
  else
    return 0;
}

static uint64_t MC_dwarf_attr_integrate_uint(Dwarf_Die * die, int attribute,
                                             uint64_t default_value)
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate(die, attribute, &attr) == NULL)
    return default_value;
  Dwarf_Word value;
  return dwarf_formudata(dwarf_attr_integrate(die, attribute, &attr),
                         &value) == 0 ? (uint64_t) value : default_value;
}

static bool MC_dwarf_attr_flag(Dwarf_Die * die, int attribute, bool integrate)
{
  Dwarf_Attribute attr;
  if ((integrate ? dwarf_attr_integrate(die, attribute, &attr)
       : dwarf_attr(die, attribute, &attr)) == 0)
    return false;

  bool result;
  if (dwarf_formflag(&attr, &result))
    xbt_die("Unexpected form for attribute %s", MC_dwarf_attrname(attribute));
  return result;
}

/** \brief Find the default lower bound for a given language
 *
 *  The default lower bound of an array (when DW_TAG_lower_bound
 *  is missing) depends on the language of the compilation unit.
 *
 *  \param lang Language of the compilation unit (values defined in the DWARF spec)
 *  \return     Default lower bound of an array in this compilation unit
 * */
static uint64_t MC_dwarf_default_lower_bound(int lang)
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

/** \brief Finds the number of elements in a DW_TAG_subrange_type or DW_TAG_enumeration_type DIE
 *
 *  \param die  the DIE
 *  \param unit DIE of the compilation unit
 *  \return     number of elements in the range
 * */
static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die * die,
                                                Dwarf_Die * unit)
{
  xbt_assert(dwarf_tag(die) == DW_TAG_enumeration_type
             || dwarf_tag(die) == DW_TAG_subrange_type,
             "MC_dwarf_subrange_element_count called with DIE of type %s",
             MC_dwarf_die_tagname(die));

  // Use DW_TAG_count if present:
  if (dwarf_hasattr_integrate(die, DW_AT_count)) {
    return MC_dwarf_attr_integrate_uint(die, DW_AT_count, 0);
  }
  // Otherwise compute DW_TAG_upper_bound-DW_TAG_lower_bound + 1:

  if (!dwarf_hasattr_integrate(die, DW_AT_upper_bound)) {
    // This is not really 0, but the code expects this (we do not know):
    return 0;
  }
  uint64_t upper_bound =
      MC_dwarf_attr_integrate_uint(die, DW_AT_upper_bound, -1);

  uint64_t lower_bound = 0;
  if (dwarf_hasattr_integrate(die, DW_AT_lower_bound)) {
    lower_bound = MC_dwarf_attr_integrate_uint(die, DW_AT_lower_bound, -1);
  } else {
    lower_bound = MC_dwarf_default_lower_bound(dwarf_srclang(unit));
  }
  return upper_bound - lower_bound + 1;
}

/** \brief Finds the number of elements in a array type (DW_TAG_array_type)
 *
 *  The compilation unit might be needed because the default lower
 *  bound depends on the language of the compilation unit.
 *
 *  \param die the DIE of the DW_TAG_array_type
 *  \param unit the DIE of the compilation unit
 *  \return number of elements in this array type
 * */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die * die, Dwarf_Die * unit)
{
  xbt_assert(dwarf_tag(die) == DW_TAG_array_type,
             "MC_dwarf_array_element_count called with DIE of type %s",
             MC_dwarf_die_tagname(die));

  int result = 1;
  Dwarf_Die child;
  int res;
  for (res = dwarf_child(die, &child); res == 0;
       res = dwarf_siblingof(&child, &child)) {
    int child_tag = dwarf_tag(&child);
    if (child_tag == DW_TAG_subrange_type
        || child_tag == DW_TAG_enumeration_type) {
      result *= MC_dwarf_subrange_element_count(&child, unit);
    }
  }
  return result;
}

// ***** mc_type_t

/** \brief Initialize the location of a member of a type
 * (DW_AT_data_member_location of a DW_TAG_member).
 *
 *  \param  type   a type (struct, class)
 *  \param  member the member of the type
 *  \param  child  DIE of the member (DW_TAG_member)
 */
static void MC_dwarf_fill_member_location(mc_type_t type, mc_type_t member,
                                          Dwarf_Die * child)
{
  if (dwarf_hasattr(child, DW_AT_data_bit_offset)) {
    xbt_die("Can't groke DW_AT_data_bit_offset.");
  }

  if (!dwarf_hasattr_integrate(child, DW_AT_data_member_location)) {
    if (type->type != DW_TAG_union_type) {
      xbt_die
          ("Missing DW_AT_data_member_location field in DW_TAG_member %s of type <%"
           PRIx64 ">%s", member->name.c_str(),
           (uint64_t) type->id, type->name.c_str());
    } else {
      return;
    }
  }

  Dwarf_Attribute attr;
  dwarf_attr_integrate(child, DW_AT_data_member_location, &attr);
  int form = dwarf_whatform(&attr);
  int klass = MC_dwarf_form_get_class(form);
  switch (klass) {
  case MC_DW_CLASS_EXPRLOC:
  case MC_DW_CLASS_BLOCK:
    // Location expression:
    {
      Dwarf_Op *expr;
      size_t len;
      if (dwarf_getlocation(&attr, &expr, &len)) {
        xbt_die
            ("Could not read location expression DW_AT_data_member_location in DW_TAG_member %s of type <%"
             PRIx64 ">%s", MC_dwarf_attr_integrate_string(child, DW_AT_name),
             (uint64_t) type->id, type->name.c_str());
      }
      if (len == 1 && expr[0].atom == DW_OP_plus_uconst) {
        member->offset = expr[0].number;
      } else {
        mc_dwarf_expression_init(&member->location, len, expr);
      }
      break;
    }
  case MC_DW_CLASS_CONSTANT:
    // Offset from the base address of the object:
    {
      Dwarf_Word offset;
      if (!dwarf_formudata(&attr, &offset))
        member->offset = offset;
      else
        xbt_die("Cannot get %s location <%" PRIx64 ">%s",
                MC_dwarf_attr_integrate_string(child, DW_AT_name),
                (uint64_t) type->id, type->name.c_str());
      break;
    }
  case MC_DW_CLASS_LOCLISTPTR:
    // Reference to a location list:
    // TODO
  case MC_DW_CLASS_REFERENCE:
    // It's supposed to be possible in DWARF2 but I couldn't find its semantic
    // in the spec.
  default:
    xbt_die("Can't handle form class (%i) / form 0x%x as DW_AT_member_location",
            klass, form);
  }

}

static void dw_type_free_voidp(void *t)
{
  delete *(mc_type_t*)t;
}

/** \brief Populate the list of members of a type
 *
 *  \param info ELF object containing the type DIE
 *  \param die  DIE of the type
 *  \param unit DIE of the compilation unit containing the type DIE
 *  \param type the type
 */
static void MC_dwarf_add_members(mc_object_info_t info, Dwarf_Die * die,
                                 Dwarf_Die * unit, mc_type_t type)
{
  int res;
  Dwarf_Die child;
  xbt_assert(!type->members);
  type->members =
      xbt_dynar_new(sizeof(mc_type_t), (void (*)(void *)) dw_type_free_voidp);
  for (res = dwarf_child(die, &child); res == 0;
       res = dwarf_siblingof(&child, &child)) {
    int tag = dwarf_tag(&child);
    if (tag == DW_TAG_member || tag == DW_TAG_inheritance) {

      // Skip declarations:
      if (MC_dwarf_attr_flag(&child, DW_AT_declaration, false))
        continue;

      // Skip compile time constants:
      if (dwarf_hasattr(&child, DW_AT_const_value))
        continue;

      // TODO, we should use another type (because is is not a type but a member)
      mc_type_t member = new simgrid::mc::Type();
      member->type = tag;

      // Global Offset:
      member->id = dwarf_dieoffset(&child);

      const char *name = MC_dwarf_attr_integrate_string(&child, DW_AT_name);
      if (name)
        member->name = name;
      member->byte_size =
          MC_dwarf_attr_integrate_uint(&child, DW_AT_byte_size, 0);
      member->element_count = -1;

      char* type_id = MC_dwarf_at_type(&child);
      if (type_id) {
        member->dw_type_id = type_id;
        free(type_id);
      }

      if (dwarf_hasattr(&child, DW_AT_data_bit_offset)) {
        xbt_die("Can't groke DW_AT_data_bit_offset.");
      }

      MC_dwarf_fill_member_location(type, member, &child);

      if (member->dw_type_id.empty()) {
        xbt_die("Missing type for member %s of <%" PRIx64 ">%s",
                member->name.c_str(),
                (uint64_t) type->id, type->name.c_str());
      }

      xbt_dynar_push(type->members, &member);
    }
  }
}

/** \brief Create a MC type object from a DIE
 *
 *  \param info current object info object
 *  \param DIE (for a given type);
 *  \param unit compilation unit of the current DIE
 *  \return MC representation of the type
 */
static mc_type_t MC_dwarf_die_to_type(mc_object_info_t info, Dwarf_Die * die,
                                      Dwarf_Die * unit, dw_frame_t frame,
                                      const char *ns)
{

  mc_type_t type = new simgrid::mc::Type();
  type->type = -1;
  type->name = std::string();
  type->element_count = -1;

  type->type = dwarf_tag(die);

  // Global Offset
  type->id = dwarf_dieoffset(die);

  const char *prefix = "";
  switch (type->type) {
  case DW_TAG_structure_type:
    prefix = "struct ";
    break;
  case DW_TAG_union_type:
    prefix = "union ";
    break;
  case DW_TAG_class_type:
    prefix = "class ";
    break;
  default:
    prefix = "";
  }

  const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  if (name != NULL) {
    char* full_name = ns ? bprintf("%s%s::%s", prefix, ns, name) :
      bprintf("%s%s", prefix, name);
    type->name = std::string(full_name);
    free(full_name);
  }

  char* type_id = MC_dwarf_at_type(die);
  if (type_id) {
    type->dw_type_id = type_id;
    free(type_id);
  }

  // Some compilers do not emit DW_AT_byte_size for pointer_type,
  // so we fill this. We currently assume that the model-checked process is in
  // the same architecture..
  if (type->type == DW_TAG_pointer_type)
    type->byte_size = sizeof(void*);

  // Computation of the byte_size;
  if (dwarf_hasattr_integrate(die, DW_AT_byte_size))
    type->byte_size = MC_dwarf_attr_integrate_uint(die, DW_AT_byte_size, 0);
  else if (type->type == DW_TAG_array_type
           || type->type == DW_TAG_structure_type
           || type->type == DW_TAG_class_type) {
    Dwarf_Word size;
    if (dwarf_aggregate_size(die, &size) == 0) {
      type->byte_size = size;
    }
  }

  switch (type->type) {
  case DW_TAG_array_type:
    type->element_count = MC_dwarf_array_element_count(die, unit);
    // TODO, handle DW_byte_stride and (not) DW_bit_stride
    break;

  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
    type->is_pointer_type = 1;
    break;

  case DW_TAG_structure_type:
  case DW_TAG_union_type:
  case DW_TAG_class_type:
    MC_dwarf_add_members(info, die, unit, type);
    char *new_ns = ns == NULL ? xbt_strdup(type->name.c_str())
        : bprintf("%s::%s", ns, name);
    MC_dwarf_handle_children(info, die, unit, frame, new_ns);
    free(new_ns);
    break;
  }

  return type;
}

static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die * die,
                                     Dwarf_Die * unit, dw_frame_t frame,
                                     const char *ns)
{
  mc_type_t type = MC_dwarf_die_to_type(info, die, unit, frame, ns);

  char *key = bprintf("%" PRIx64, (uint64_t) type->id);
  xbt_dict_set(info->types, key, type, NULL);
  xbt_free(key);

  if (!type->name.empty() && type->byte_size != 0) {
    xbt_dict_set(info->full_types_by_name, type->name.c_str(), type, NULL);
  }
}

static int mc_anonymous_variable_index = 0;

static dw_variable_t MC_die_to_variable(mc_object_info_t info, Dwarf_Die * die,
                                        Dwarf_Die * unit, dw_frame_t frame,
                                        const char *ns)
{
  // Skip declarations:
  if (MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return NULL;

  // Skip compile time constants:
  if (dwarf_hasattr(die, DW_AT_const_value))
    return NULL;

  Dwarf_Attribute attr_location;
  if (dwarf_attr(die, DW_AT_location, &attr_location) == NULL) {
    // No location: do not add it ?
    return NULL;
  }

  dw_variable_t variable = xbt_new0(s_dw_variable_t, 1);
  variable->dwarf_offset = dwarf_dieoffset(die);
  variable->global = frame == NULL;     // Can be override base on DW_AT_location
  variable->object_info = info;

  const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  variable->name = xbt_strdup(name);

  variable->type_origin = MC_dwarf_at_type(die);

  int form = dwarf_whatform(&attr_location);
  int klass =
      form ==
      DW_FORM_sec_offset ? MC_DW_CLASS_CONSTANT : MC_dwarf_form_get_class(form);
  switch (klass) {
  case MC_DW_CLASS_EXPRLOC:
  case MC_DW_CLASS_BLOCK:
    // Location expression:
    {
      Dwarf_Op *expr;
      size_t len;
      if (dwarf_getlocation(&attr_location, &expr, &len)) {
        xbt_die
            ("Could not read location expression in DW_AT_location of variable <%"
             PRIx64 ">%s", (uint64_t) variable->dwarf_offset, variable->name);
      }

      if (len == 1 && expr[0].atom == DW_OP_addr) {
        variable->global = 1;
        uintptr_t offset = (uintptr_t) expr[0].number;
        uintptr_t base = (uintptr_t) info->base_address();
        variable->address = (void *) (base + offset);
      } else {
        mc_dwarf_location_list_init_from_expression(&variable->locations, len,
                                                    expr);
      }

      break;
    }
  case MC_DW_CLASS_LOCLISTPTR:
  case MC_DW_CLASS_CONSTANT:
    // Reference to location list:
    mc_dwarf_location_list_init(&variable->locations, info, die,
                                &attr_location);
    break;
  default:
    xbt_die("Unexpected form 0x%x (%i), class 0x%x (%i) list for location in <%"
            PRIx64 ">%s", form, form, klass, klass,
            (uint64_t) variable->dwarf_offset, variable->name);
  }

  // Handle start_scope:
  if (dwarf_hasattr(die, DW_AT_start_scope)) {
    Dwarf_Attribute attr;
    dwarf_attr(die, DW_AT_start_scope, &attr);
    int form = dwarf_whatform(&attr);
    int klass = MC_dwarf_form_get_class(form);
    switch (klass) {
    case MC_DW_CLASS_CONSTANT:
      {
        Dwarf_Word value;
        variable->start_scope =
            dwarf_formudata(&attr, &value) == 0 ? (size_t) value : 0;
        break;
      }
    case MC_DW_CLASS_RANGELISTPTR:     // TODO
    default:
      xbt_die
          ("Unhandled form 0x%x, class 0x%X for DW_AT_start_scope of variable %s",
           form, klass, name == NULL ? "?" : name);
    }
  }

  if (ns && variable->global) {
    char *old_name = variable->name;
    variable->name = bprintf("%s::%s", ns, old_name);
    free(old_name);
  }
  // The current code needs a variable name,
  // generate a fake one:
  if (!variable->name) {
    variable->name = bprintf("@anonymous#%i", mc_anonymous_variable_index++);
  }

  return variable;
}

static void MC_dwarf_handle_variable_die(mc_object_info_t info, Dwarf_Die * die,
                                         Dwarf_Die * unit, dw_frame_t frame,
                                         const char *ns)
{
  dw_variable_t variable =
      MC_die_to_variable(info, die, unit, frame, ns);
  if (variable == NULL)
    return;
  MC_dwarf_register_variable(info, frame, variable);
}

static void mc_frame_free_voipd(dw_frame_t * p)
{
  mc_frame_free(*p);
  *p = NULL;
}

static void MC_dwarf_handle_scope_die(mc_object_info_t info, Dwarf_Die * die,
                                      Dwarf_Die * unit, dw_frame_t parent_frame,
                                      const char *ns)
{
  // TODO, handle DW_TAG_type/DW_TAG_location for DW_TAG_with_stmt
  int tag = dwarf_tag(die);
  mc_tag_class klass = MC_dwarf_tag_classify(tag);

  // (Template) Subprogram declaration:
  if (klass == mc_tag_subprogram
      && MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return;

  if (klass == mc_tag_scope)
    xbt_assert(parent_frame, "No parent scope for this scope");

  dw_frame_t frame = xbt_new0(s_dw_frame_t, 1);

  frame->tag = tag;
  frame->id = dwarf_dieoffset(die);
  frame->object_info = info;

  if (klass == mc_tag_subprogram) {
    const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
    frame->name =
        ns ? bprintf("%s::%s", ns, name) : xbt_strdup(name);
  }

  frame->abstract_origin_id =
      MC_dwarf_attr_dieoffset(die, DW_AT_abstract_origin);

  // This is the base address for DWARF addresses.
  // Relocated addresses are offset from this base address.
  // See DWARF4 spec 7.5
  void *base = info->base_address();

  // Variables are filled in the (recursive) call of MC_dwarf_handle_children:
  frame->variables =
      xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);

  // TODO, support DW_AT_ranges
  uint64_t low_pc = MC_dwarf_attr_integrate_addr(die, DW_AT_low_pc);
  frame->low_pc = low_pc ? ((char *) base) + low_pc : 0;
  if (low_pc) {
    // DW_AT_high_pc:
    Dwarf_Attribute attr;
    if (!dwarf_attr_integrate(die, DW_AT_high_pc, &attr)) {
      xbt_die("Missing DW_AT_high_pc matching with DW_AT_low_pc");
    }

    Dwarf_Sword offset;
    Dwarf_Addr high_pc;

    switch (MC_dwarf_form_get_class(dwarf_whatform(&attr))) {

      // DW_AT_high_pc if an offset from the low_pc:
    case MC_DW_CLASS_CONSTANT:

      if (dwarf_formsdata(&attr, &offset) != 0)
        xbt_die("Could not read constant");
      frame->high_pc = (void *) ((char *) frame->low_pc + offset);
      break;

      // DW_AT_high_pc is a relocatable address:
    case MC_DW_CLASS_ADDRESS:
      if (dwarf_formaddr(&attr, &high_pc) != 0)
        xbt_die("Could not read address");
      frame->high_pc = ((char *) base) + high_pc;
      break;

    default:
      xbt_die("Unexpected class for DW_AT_high_pc");

    }
  }

  if (klass == mc_tag_subprogram) {
    Dwarf_Attribute attr_frame_base;
    if (dwarf_attr_integrate(die, DW_AT_frame_base, &attr_frame_base))
      mc_dwarf_location_list_init(&frame->frame_base, info, die,
                                  &attr_frame_base);
  }

  frame->scopes =
      xbt_dynar_new(sizeof(dw_frame_t), (void_f_pvoid_t) mc_frame_free_voipd);

  // Register it:
  if (klass == mc_tag_subprogram) {
    char *key = bprintf("%" PRIx64, (uint64_t) frame->id);
    xbt_dict_set(info->subprograms, key, frame, NULL);
    xbt_free(key);
  } else if (klass == mc_tag_scope) {
    xbt_dynar_push(parent_frame->scopes, &frame);
  }
  // Handle children:
  MC_dwarf_handle_children(info, die, unit, frame, ns);
}

static void mc_dwarf_handle_namespace_die(mc_object_info_t info,
                                          Dwarf_Die * die, Dwarf_Die * unit,
                                          dw_frame_t frame,
                                          const char *ns)
{
  const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  if (frame)
    xbt_die("Unexpected namespace in a subprogram");
  char *new_ns = ns == NULL ? xbt_strdup(name)
      : bprintf("%s::%s", ns, name);
  MC_dwarf_handle_children(info, die, unit, frame, new_ns);
  xbt_free(new_ns);
}

static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die * die,
                                     Dwarf_Die * unit, dw_frame_t frame,
                                     const char *ns)
{
  // For each child DIE:
  Dwarf_Die child;
  int res;
  for (res = dwarf_child(die, &child); res == 0;
       res = dwarf_siblingof(&child, &child)) {
    MC_dwarf_handle_die(info, &child, unit, frame, ns);
  }
}

static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die * die,
                                Dwarf_Die * unit, dw_frame_t frame,
                                const char *ns)
{
  int tag = dwarf_tag(die);
  mc_tag_class klass = MC_dwarf_tag_classify(tag);
  switch (klass) {

    // Type:
  case mc_tag_type:
    MC_dwarf_handle_type_die(info, die, unit, frame, ns);
    break;

    // Subprogram or scope:
  case mc_tag_subprogram:
  case mc_tag_scope:
    MC_dwarf_handle_scope_die(info, die, unit, frame, ns);
    return;

    // Variable:
  case mc_tag_variable:
    MC_dwarf_handle_variable_die(info, die, unit, frame, ns);
    break;

  case mc_tag_namespace:
    mc_dwarf_handle_namespace_die(info, die, unit, frame, ns);
    break;

  default:
    break;

  }
}

/** \brief Populate the debugging informations of the given ELF object
 *
 *  Read the DWARf information of the EFFL object and populate the
 *  lists of types, variables, functions.
 */
void MC_dwarf_get_variables(mc_object_info_t info)
{
  int fd = open(info->file_name, O_RDONLY);
  if (fd < 0) {
    xbt_die("Could not open file %s", info->file_name);
  }
  Dwarf *dwarf = dwarf_begin(fd, DWARF_C_READ);
  if (dwarf == NULL) {
    xbt_die("Your program must be compiled with -g (%s)", info->file_name);
  }
  // For each compilation unit:
  Dwarf_Off offset = 0;
  Dwarf_Off next_offset = 0;
  size_t length;
  while (dwarf_nextcu(dwarf, offset, &next_offset, &length, NULL, NULL, NULL) ==
         0) {
    Dwarf_Die unit_die;
    if (dwarf_offdie(dwarf, offset + length, &unit_die) != NULL) {

      // For each child DIE:
      Dwarf_Die child;
      int res;
      for (res = dwarf_child(&unit_die, &child); res == 0;
           res = dwarf_siblingof(&child, &child)) {
        MC_dwarf_handle_die(info, &child, &unit_die, NULL, NULL);
      }

    }
    offset = next_offset;
  }

  dwarf_end(dwarf);
  close(fd);
}

/************************** Free functions *************************/

void mc_frame_free(dw_frame_t frame)
{
  xbt_free(frame->name);
  mc_dwarf_location_list_clear(&(frame->frame_base));
  xbt_dynar_free(&(frame->variables));
  xbt_dynar_free(&(frame->scopes));
  xbt_free(frame);
}

static void dw_type_free(mc_type_t t)
{
  delete t;
}

void dw_variable_free(dw_variable_t v)
{
  if (v) {
    xbt_free(v->name);
    xbt_free(v->type_origin);

    if (v->locations.locations)
      mc_dwarf_location_list_clear(&v->locations);
    xbt_free(v);
  }
}

void dw_variable_free_voidp(void *t)
{
  dw_variable_free((dw_variable_t) * (void **) t);
}

// ***** object_info

namespace simgrid {
namespace mc {

ObjectInformation::ObjectInformation()
{
  this->flags = 0;
  this->file_name = nullptr;
  this->start = nullptr;
  this->end = nullptr;
  this->start_exec = nullptr;
  this->end_exec = nullptr;
  this->start_rw = nullptr;
  this->end_rw = nullptr;
  this->start_ro = nullptr;
  this->end_ro = nullptr;
  this->subprograms = xbt_dict_new_homogeneous((void (*)(void *)) mc_frame_free);
  this->global_variables =
      xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
  this->types = xbt_dict_new_homogeneous((void (*)(void *)) dw_type_free);
  this->full_types_by_name = xbt_dict_new_homogeneous(NULL);
  this->functions_index = nullptr;
}

ObjectInformation::~ObjectInformation()
{
  xbt_free(this->file_name);
  xbt_dict_free(&this->subprograms);
  xbt_dynar_free(&this->global_variables);
  xbt_dict_free(&this->types);
  xbt_dict_free(&this->full_types_by_name);
  xbt_dynar_free(&this->functions_index);
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
void *ObjectInformation::base_address() const
{
  if (this->executable())
    return nullptr;

  void *result = this->start_exec;
  if (this->start_rw != NULL && result > (void *) this->start_rw)
    result = this->start_rw;
  if (this->start_ro != NULL && result > (void *) this->start_ro)
    result = this->start_ro;
  return result;
}

}
}

// ***** Helpers

// ***** Functions index

static int MC_compare_frame_index_items(mc_function_index_item_t a,
                                        mc_function_index_item_t b)
{
  if (a->low_pc < b->low_pc)
    return -1;
  else if (a->low_pc == b->low_pc)
    return 0;
  else
    return 1;
}

static void MC_make_functions_index(mc_object_info_t info)
{
  xbt_dynar_t index = xbt_dynar_new(sizeof(s_mc_function_index_item_t), NULL);

  // Populate the array:
  dw_frame_t frame = NULL;
  xbt_dict_cursor_t cursor;
  char *key;
  xbt_dict_foreach(info->subprograms, cursor, key, frame) {
    if (frame->low_pc == NULL)
      continue;
    s_mc_function_index_item_t entry;
    entry.low_pc = frame->low_pc;
    entry.high_pc = frame->high_pc;
    entry.function = frame;
    xbt_dynar_push(index, &entry);
  }

  mc_function_index_item_t base =
      (mc_function_index_item_t) xbt_dynar_get_ptr(index, 0);

  // Sort the array by low_pc:
  qsort(base,
        xbt_dynar_length(index),
        sizeof(s_mc_function_index_item_t),
        (int (*)(const void *, const void *)) MC_compare_frame_index_items);

  info->functions_index = index;
}

static void MC_post_process_variables(mc_object_info_t info)
{
  unsigned cursor = 0;
  dw_variable_t variable = NULL;
  xbt_dynar_foreach(info->global_variables, cursor, variable) {
    if (variable->type_origin) {
      variable->type = (mc_type_t) xbt_dict_get_or_null(info->types, variable->type_origin);
    }
  }
}

static void mc_post_process_scope(mc_object_info_t info, dw_frame_t scope)
{

  if (scope->tag == DW_TAG_inlined_subroutine) {

    // Attach correct namespaced name in inlined subroutine:
    char *key = bprintf("%" PRIx64, (uint64_t) scope->abstract_origin_id);
    dw_frame_t abstract_origin = (dw_frame_t) xbt_dict_get_or_null(info->subprograms, key);
    xbt_assert(abstract_origin, "Could not lookup abstract origin %s", key);
    xbt_free(key);
    scope->name = xbt_strdup(abstract_origin->name);

  }
  // Direct:
  unsigned cursor = 0;
  dw_variable_t variable = NULL;
  xbt_dynar_foreach(scope->variables, cursor, variable) {
    if (variable->type_origin) {
      variable->type = (mc_type_t) xbt_dict_get_or_null(info->types, variable->type_origin);
    }
  }

  // Recursive post-processing of nested-scopes:
  dw_frame_t nested_scope = NULL;
  xbt_dynar_foreach(scope->scopes, cursor, nested_scope)
      mc_post_process_scope(info, nested_scope);

}

static void MC_post_process_functions(mc_object_info_t info)
{
  xbt_dict_cursor_t cursor;
  char *key;
  dw_frame_t subprogram = NULL;
  xbt_dict_foreach(info->subprograms, cursor, key, subprogram) {
    mc_post_process_scope(info, subprogram);
  }
}


/** \brief Fill/lookup the "subtype" field.
 */
static void MC_resolve_subtype(mc_object_info_t info, mc_type_t type)
{

  if (type->dw_type_id.empty())
    return;
  type->subtype = (mc_type_t) xbt_dict_get_or_null(
    info->types, type->dw_type_id.c_str());
  if (type->subtype == NULL)
    return;
  if (type->subtype->byte_size != 0)
    return;
  if (type->subtype->name.empty())
    return;
  // Try to find a more complete description of the type:
  // We need to fix in order to support C++.

  mc_type_t subtype =
    (mc_type_t) xbt_dict_get_or_null(
      info->full_types_by_name, type->subtype->name.c_str());
  if (subtype != NULL) {
    type->subtype = subtype;
  }

}

static void MC_post_process_types(mc_object_info_t info)
{
  xbt_dict_cursor_t cursor = NULL;
  char *origin;
  mc_type_t type;

  // Lookup "subtype" field:
  xbt_dict_foreach(info->types, cursor, origin, type) {
    MC_resolve_subtype(info, type);

    mc_type_t member;
    unsigned int i = 0;
    if (type->members != NULL)
      xbt_dynar_foreach(type->members, i, member) {
      MC_resolve_subtype(info, member);
      }
  }
}

/** \brief Finds informations about a given shared object/executable */
std::shared_ptr<s_mc_object_info_t> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char *name, int executable)
{
  std::shared_ptr<s_mc_object_info_t> result =
    std::make_shared<s_mc_object_info_t>();
  if (executable)
    result->flags |= MC_OBJECT_INFO_EXECUTABLE;
  result->file_name = xbt_strdup(name);
  MC_find_object_address(maps, result.get());
  MC_dwarf_get_variables(result.get());
  MC_post_process_types(result.get());
  MC_post_process_variables(result.get());
  MC_post_process_functions(result.get());
  MC_make_functions_index(result.get());
  return std::move(result);
}

/*************************************************************************/

static int MC_dwarf_get_variable_index(xbt_dynar_t variables, char *var,
                                       void *address)
{

  if (xbt_dynar_is_empty(variables))
    return 0;

  unsigned int cursor = 0;
  int start = 0;
  int end = xbt_dynar_length(variables) - 1;
  dw_variable_t var_test = NULL;

  while (start <= end) {
    cursor = (start + end) / 2;
    var_test =
        (dw_variable_t) xbt_dynar_get_as(variables, cursor, dw_variable_t);
    if (strcmp(var_test->name, var) < 0) {
      start = cursor + 1;
    } else if (strcmp(var_test->name, var) > 0) {
      end = cursor - 1;
    } else {
      if (address) {            /* global variable */
        if (var_test->address == address)
          return -1;
        if (var_test->address > address)
          end = cursor - 1;
        else
          start = cursor + 1;
      } else {                  /* local variable */
        return -1;
      }
    }
  }

  if (strcmp(var_test->name, var) == 0) {
    if (address && var_test->address < address)
      return cursor + 1;
    else
      return cursor;
  } else if (strcmp(var_test->name, var) < 0)
    return cursor + 1;
  else
    return cursor;

}

void MC_dwarf_register_global_variable(mc_object_info_t info,
                                       dw_variable_t variable)
{
  int index =
      MC_dwarf_get_variable_index(info->global_variables, variable->name,
                                  variable->address);
  if (index != -1)
    xbt_dynar_insert_at(info->global_variables, index, &variable);
  // TODO, else ?
}

void MC_dwarf_register_non_global_variable(mc_object_info_t info,
                                           dw_frame_t frame,
                                           dw_variable_t variable)
{
  xbt_assert(frame, "Frame is NULL");
  int index =
      MC_dwarf_get_variable_index(frame->variables, variable->name, NULL);
  if (index != -1)
    xbt_dynar_insert_at(frame->variables, index, &variable);
  // TODO, else ?
}

void MC_dwarf_register_variable(mc_object_info_t info, dw_frame_t frame,
                                dw_variable_t variable)
{
  if (variable->global)
    MC_dwarf_register_global_variable(info, variable);
  else if (frame == NULL)
    xbt_die("No frame for this local variable");
  else
    MC_dwarf_register_non_global_variable(info, frame, variable);
}

void MC_post_process_object_info(mc_process_t process, mc_object_info_t info)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key = NULL;
  mc_type_t type = NULL;
  xbt_dict_foreach(info->types, cursor, key, type) {

    mc_type_t subtype = type;
    while (subtype->type == DW_TAG_typedef || subtype->type == DW_TAG_volatile_type
      || subtype->type == DW_TAG_const_type) {
      if (subtype->subtype)
        subtype = subtype->subtype;
      else
        break;
    }

    // Resolve full_type:
    if (!subtype->name.empty() && subtype->byte_size == 0) {
      for (auto const& object_info : process->object_infos) {
        mc_type_t same_type = (mc_type_t)
            xbt_dict_get_or_null(object_info->full_types_by_name,
                                 subtype->name.c_str());
        if (same_type && !same_type->name.empty() && same_type->byte_size) {
          type->full_type = same_type;
          break;
        }
      }
    } else type->full_type = subtype;

  }
}
