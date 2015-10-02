/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>
#include <cstdint>

#include <algorithm>
#include <memory>

#include <stdlib.h>
#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */
#include <dwarf.h>
#include <elfutils/libdw.h>

#include <simgrid_config.h>
#include <simgrid/util.hpp>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include <simgrid/util.hpp>

#include "mc_object_info.h"
#include "mc_private.h"
#include "mc_process.h"

#include "mc/ObjectInformation.hpp"
#include "mc/Variable.hpp"

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
static void MC_dwarf_handle_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                const char *ns);

/** \brief Process a type DIE
 */
static void MC_dwarf_handle_type_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                     const char *ns);

/** \brief Calls MC_dwarf_handle_die on all childrend of the given die
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_children(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                     const char *ns);

/** \brief Handle a variable (DW_TAG_variable or other)
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_variable_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                         Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                         const char *ns);

/** \brief Get the DW_TAG_type of the DIE
 *
 *  \param die DIE
 *  \return DW_TAG_type attribute as a new string (NULL if none)
 */
static std::uint64_t MC_dwarf_at_type(Dwarf_Die * die);

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
  if (dwarf_hasattr_integrate(die, attribute) == 0)
    return 0;
  dwarf_attr_integrate(die, attribute, &attr);
  Dwarf_Die subtype_die;
  if (dwarf_formref_die(&attr, &subtype_die) == NULL)
    xbt_die("Could not find DIE");
  return dwarf_dieoffset(&subtype_die);
}

static Dwarf_Off MC_dwarf_attr_integrate_dieoffset(Dwarf_Die * die,
                                                   int attribute)
{
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, attribute) == 0)
    return 0;
  dwarf_attr_integrate(die, DW_AT_type, &attr);
  Dwarf_Die subtype_die;
  if (dwarf_formref_die(&attr, &subtype_die) == NULL)
    xbt_die("Could not find DIE");
  return dwarf_dieoffset(&subtype_die);
}

/** \brief Find the type/subtype (DW_AT_type) for a DIE
 *
 *  \param dit the DIE
 *  \return DW_AT_type reference as a global offset in hexadecimal (or NULL)
 */
static
std::uint64_t MC_dwarf_at_type(Dwarf_Die * die)
{
  return MC_dwarf_attr_integrate_dieoffset(die, DW_AT_type);
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
  if (dwarf_hasattr_integrate(die, DW_AT_count))
    return MC_dwarf_attr_integrate_uint(die, DW_AT_count, 0);
  // Otherwise compute DW_TAG_upper_bound-DW_TAG_lower_bound + 1:

  if (!dwarf_hasattr_integrate(die, DW_AT_upper_bound))
    // This is not really 0, but the code expects this (we do not know):
    return 0;

  uint64_t upper_bound =
      MC_dwarf_attr_integrate_uint(die, DW_AT_upper_bound, -1);

  uint64_t lower_bound = 0;
  if (dwarf_hasattr_integrate(die, DW_AT_lower_bound))
    lower_bound = MC_dwarf_attr_integrate_uint(die, DW_AT_lower_bound, -1);
  else
    lower_bound = MC_dwarf_default_lower_bound(dwarf_srclang(unit));
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
        || child_tag == DW_TAG_enumeration_type)
      result *= MC_dwarf_subrange_element_count(&child, unit);
  }
  return result;
}

// ***** Variable

/** Sort the variable by name and address.
 *
 *  We could use boost::container::flat_set instead.
 */
static bool MC_compare_variable(
  simgrid::mc::Variable const& a, simgrid::mc::Variable const& b)
{
  int cmp = strcmp(a.name.c_str(), b.name.c_str());
  if (cmp < 0)
    return true;
  else if (cmp > 0)
    return false;
  else
    return a.address < b.address;
}

// ***** simgrid::mc::Type*

/** \brief Initialize the location of a member of a type
 * (DW_AT_data_member_location of a DW_TAG_member).
 *
 *  \param  type   a type (struct, class)
 *  \param  member the member of the type
 *  \param  child  DIE of the member (DW_TAG_member)
 */
static void MC_dwarf_fill_member_location(simgrid::mc::Type* type, simgrid::mc::Type* member,
                                          Dwarf_Die * child)
{
  if (dwarf_hasattr(child, DW_AT_data_bit_offset))
    xbt_die("Can't groke DW_AT_data_bit_offset.");

  if (!dwarf_hasattr_integrate(child, DW_AT_data_member_location)) {
    if (type->type == DW_TAG_union_type)
      return;
    xbt_die
        ("Missing DW_AT_data_member_location field in DW_TAG_member %s of type <%"
         PRIx64 ">%s", member->name.c_str(),
         (uint64_t) type->id, type->name.c_str());
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
      if (dwarf_getlocation(&attr, &expr, &len))
        xbt_die
            ("Could not read location expression DW_AT_data_member_location in DW_TAG_member %s of type <%"
             PRIx64 ">%s", MC_dwarf_attr_integrate_string(child, DW_AT_name),
             (uint64_t) type->id, type->name.c_str());
      simgrid::mc::DwarfExpression(expr, expr+len);
      break;
    }
  case MC_DW_CLASS_CONSTANT:
    // Offset from the base address of the object:
    {
      Dwarf_Word offset;
      if (!dwarf_formudata(&attr, &offset))
        member->offset(offset);
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

/** \brief Populate the list of members of a type
 *
 *  \param info ELF object containing the type DIE
 *  \param die  DIE of the type
 *  \param unit DIE of the compilation unit containing the type DIE
 *  \param type the type
 */
static void MC_dwarf_add_members(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                 Dwarf_Die * unit, simgrid::mc::Type* type)
{
  int res;
  Dwarf_Die child;
  xbt_assert(type->members.empty());
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
      simgrid::mc::Type member;
      member.type = tag;

      // Global Offset:
      member.id = dwarf_dieoffset(&child);

      const char *name = MC_dwarf_attr_integrate_string(&child, DW_AT_name);
      if (name)
        member.name = name;
      member.byte_size =
          MC_dwarf_attr_integrate_uint(&child, DW_AT_byte_size, 0);
      member.element_count = -1;
      member.type_id = MC_dwarf_at_type(&child);

      if (dwarf_hasattr(&child, DW_AT_data_bit_offset))
        xbt_die("Can't groke DW_AT_data_bit_offset.");

      MC_dwarf_fill_member_location(type, &member, &child);

      if (!member.type_id)
        xbt_die("Missing type for member %s of <%" PRIx64 ">%s",
                member.name.c_str(),
                (uint64_t) type->id, type->name.c_str());

      type->members.push_back(std::move(member));
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
static simgrid::mc::Type MC_dwarf_die_to_type(
  simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
  Dwarf_Die * unit, simgrid::mc::Frame* frame,
  const char *ns)
{
  simgrid::mc::Type type;
  type.type = dwarf_tag(die);
  type.name = std::string();
  type.element_count = -1;

  // Global Offset
  type.id = dwarf_dieoffset(die);

  const char *prefix = "";
  switch (type.type) {
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
    type.name = std::string(full_name);
    free(full_name);
  }

  type.type_id = MC_dwarf_at_type(die);

  // Some compilers do not emit DW_AT_byte_size for pointer_type,
  // so we fill this. We currently assume that the model-checked process is in
  // the same architecture..
  if (type.type == DW_TAG_pointer_type)
    type.byte_size = sizeof(void*);

  // Computation of the byte_size;
  if (dwarf_hasattr_integrate(die, DW_AT_byte_size))
    type.byte_size = MC_dwarf_attr_integrate_uint(die, DW_AT_byte_size, 0);
  else if (type.type == DW_TAG_array_type
           || type.type == DW_TAG_structure_type
           || type.type == DW_TAG_class_type) {
    Dwarf_Word size;
    if (dwarf_aggregate_size(die, &size) == 0)
      type.byte_size = size;
  }

  switch (type.type) {
  case DW_TAG_array_type:
    type.element_count = MC_dwarf_array_element_count(die, unit);
    // TODO, handle DW_byte_stride and (not) DW_bit_stride
    break;

  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
    type.is_pointer_type = 1;
    break;

  case DW_TAG_structure_type:
  case DW_TAG_union_type:
  case DW_TAG_class_type:
    MC_dwarf_add_members(info, die, unit, &type);
    char *new_ns = ns == NULL ? xbt_strdup(type.name.c_str())
        : bprintf("%s::%s", ns, name);
    MC_dwarf_handle_children(info, die, unit, frame, new_ns);
    free(new_ns);
    break;
  }

  return std::move(type);
}

static void MC_dwarf_handle_type_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                     const char *ns)
{
  simgrid::mc::Type type = MC_dwarf_die_to_type(info, die, unit, frame, ns);
  auto& t = (info->types[type.id] = std::move(type));
  if (!t.name.empty() && type.byte_size != 0)
    info->full_types_by_name[t.name] = &t;
}

static int mc_anonymous_variable_index = 0;

static std::unique_ptr<simgrid::mc::Variable> MC_die_to_variable(
  simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
  Dwarf_Die * unit, simgrid::mc::Frame* frame,
  const char *ns)
{
  // Skip declarations:
  if (MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return nullptr;

  // Skip compile time constants:
  if (dwarf_hasattr(die, DW_AT_const_value))
    return nullptr;

  Dwarf_Attribute attr_location;
  if (dwarf_attr(die, DW_AT_location, &attr_location) == NULL)
    // No location: do not add it ?
    return nullptr;

  std::unique_ptr<simgrid::mc::Variable> variable =
    std::unique_ptr<simgrid::mc::Variable>(new simgrid::mc::Variable());
  variable->dwarf_offset = dwarf_dieoffset(die);
  variable->global = frame == NULL;     // Can be override base on DW_AT_location
  variable->object_info = info;

  const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
  if (name)
    variable->name = name;
  variable->type_id = MC_dwarf_at_type(die);

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
        xbt_die(
          "Could not read location expression in DW_AT_location "
          "of variable <%" PRIx64 ">%s",
          (uint64_t) variable->dwarf_offset,
          variable->name.c_str());
      }

      if (len == 1 && expr[0].atom == DW_OP_addr) {
        variable->global = 1;
        uintptr_t offset = (uintptr_t) expr[0].number;
        uintptr_t base = (uintptr_t) info->base_address();
        variable->address = (void *) (base + offset);
      } else {
        simgrid::mc::LocationListEntry entry;
        entry.expression = {expr, expr + len};
        variable->location_list = { std::move(entry) };
      }

      break;
    }
  case MC_DW_CLASS_LOCLISTPTR:
  case MC_DW_CLASS_CONSTANT:
    // Reference to location list:
    mc_dwarf_location_list_init(
      &variable->location_list, info, die,
      &attr_location);
    break;
  default:
    xbt_die("Unexpected form 0x%x (%i), class 0x%x (%i) list for location "
            "in <%" PRIx64 ">%s",
            form, form, klass, klass,
            (uint64_t) variable->dwarf_offset,
            variable->name.c_str());
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

  if (ns && variable->global)
    variable->name =
      std::string(ns) + "::" + variable->name;

  // The current code needs a variable name,
  // generate a fake one:
  if (variable->name.empty())
    variable->name =
      "@anonymous#" + std::to_string(mc_anonymous_variable_index++);

  return std::move(variable);
}

static void MC_dwarf_handle_variable_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                         Dwarf_Die * unit, simgrid::mc::Frame* frame,
                                         const char *ns)
{
  std::unique_ptr<simgrid::mc::Variable> variable =
    MC_die_to_variable(info, die, unit, frame, ns);
  if (!variable)
    return;
  // Those arrays are sorted later:
  else if (variable->global)
    info->global_variables.push_back(std::move(*variable));
  else if (frame != nullptr)
    frame->variables.push_back(std::move(*variable));
  else
    xbt_die("No frame for this local variable");
}

static void MC_dwarf_handle_scope_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                      Dwarf_Die * unit, simgrid::mc::Frame* parent_frame,
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

  simgrid::mc::Frame frame;

  frame.tag = tag;
  frame.id = dwarf_dieoffset(die);
  frame.object_info = info;

  if (klass == mc_tag_subprogram) {
    const char *name = MC_dwarf_attr_integrate_string(die, DW_AT_name);
    if(ns)
      frame.name  = std::string(ns) + "::" + name;
    else if (name)
      frame.name = name;
    else
      frame.name.clear();
  }

  frame.abstract_origin_id =
    MC_dwarf_attr_dieoffset(die, DW_AT_abstract_origin);

  // This is the base address for DWARF addresses.
  // Relocated addresses are offset from this base address.
  // See DWARF4 spec 7.5
  void *base = info->base_address();

  // TODO, support DW_AT_ranges
  uint64_t low_pc = MC_dwarf_attr_integrate_addr(die, DW_AT_low_pc);
  frame.low_pc = low_pc ? ((char *) base) + low_pc : 0;
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
      frame.high_pc = (void *) ((char *) frame.low_pc + offset);
      break;

      // DW_AT_high_pc is a relocatable address:
    case MC_DW_CLASS_ADDRESS:
      if (dwarf_formaddr(&attr, &high_pc) != 0)
        xbt_die("Could not read address");
      frame.high_pc = ((char *) base) + high_pc;
      break;

    default:
      xbt_die("Unexpected class for DW_AT_high_pc");

    }
  }

  if (klass == mc_tag_subprogram) {
    Dwarf_Attribute attr_frame_base;
    if (dwarf_attr_integrate(die, DW_AT_frame_base, &attr_frame_base))
      mc_dwarf_location_list_init(&frame.frame_base, info, die,
                                  &attr_frame_base);
  }

  // Handle children:
  MC_dwarf_handle_children(info, die, unit, &frame, ns);

  // Someone needs this to be sorted but who?
  std::sort(frame.variables.begin(), frame.variables.end(),
    MC_compare_variable);

  // Register it:
  if (klass == mc_tag_subprogram)
    info->subprograms[frame.id] = frame;
  else if (klass == mc_tag_scope)
    parent_frame->scopes.push_back(std::move(frame));
}

static void mc_dwarf_handle_namespace_die(simgrid::mc::ObjectInformation* info,
                                          Dwarf_Die * die, Dwarf_Die * unit,
                                          simgrid::mc::Frame* frame,
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

static void MC_dwarf_handle_children(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                     Dwarf_Die * unit, simgrid::mc::Frame* frame,
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

static void MC_dwarf_handle_die(simgrid::mc::ObjectInformation* info, Dwarf_Die * die,
                                Dwarf_Die * unit, simgrid::mc::Frame* frame,
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
void MC_dwarf_get_variables(simgrid::mc::ObjectInformation* info)
{
  int fd = open(info->file_name.c_str(), O_RDONLY);
  if (fd < 0)
    xbt_die("Could not open file %s", info->file_name.c_str());
  Dwarf *dwarf = dwarf_begin(fd, DWARF_C_READ);
  if (dwarf == NULL)
    xbt_die("Missing debugging information in %s\n"
      "Your program and its dependencies must have debugging information.\n"
      "You might want to recompile with -g or install the suitable debugging package.\n",
      info->file_name.c_str());
  // For each compilation unit:
  Dwarf_Off offset = 0;
  Dwarf_Off next_offset = 0;
  size_t length;

  while (dwarf_nextcu(dwarf, offset, &next_offset, &length, NULL, NULL, NULL) ==
         0) {
    Dwarf_Die unit_die;
    if (dwarf_offdie(dwarf, offset + length, &unit_die) != NULL)
      MC_dwarf_handle_children(info, &unit_die, &unit_die, NULL, NULL);
    offset = next_offset;
  }

  dwarf_end(dwarf);
  close(fd);
}

// ***** Functions index

static int MC_compare_frame_index_items(simgrid::mc::FunctionIndexEntry* a,
                                        simgrid::mc::FunctionIndexEntry* b)
{
  if (a->low_pc < b->low_pc)
    return -1;
  else if (a->low_pc == b->low_pc)
    return 0;
  else
    return 1;
}

static void MC_make_functions_index(simgrid::mc::ObjectInformation* info)
{
  info->functions_index.clear();

  for (auto& e : info->subprograms) {
    if (e.second.low_pc == nullptr)
      continue;
    simgrid::mc::FunctionIndexEntry entry;
    entry.low_pc = e.second.low_pc;
    entry.function = &e.second;
    info->functions_index.push_back(entry);
  }

  info->functions_index.shrink_to_fit();

  // Sort the array by low_pc:
  std::sort(info->functions_index.begin(), info->functions_index.end(),
        [](simgrid::mc::FunctionIndexEntry const& a,
          simgrid::mc::FunctionIndexEntry const& b)
        {
          return a.low_pc < b.low_pc;
        });
}

static void MC_post_process_variables(simgrid::mc::ObjectInformation* info)
{
  // Someone needs this to be sorted but who?
  std::sort(info->global_variables.begin(), info->global_variables.end(),
    MC_compare_variable);

  for(simgrid::mc::Variable& variable : info->global_variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(
        info->types, variable.type_id);
}

static void mc_post_process_scope(simgrid::mc::ObjectInformation* info, simgrid::mc::Frame* scope)
{

  if (scope->tag == DW_TAG_inlined_subroutine) {
    // Attach correct namespaced name in inlined subroutine:
    auto i = info->subprograms.find(scope->abstract_origin_id);
    xbt_assert(i != info->subprograms.end(),
      "Could not lookup abstract origin %" PRIx64,
      (std::uint64_t) scope->abstract_origin_id);
    scope->name = i->second.name;
  }

  // Direct:
  for (simgrid::mc::Variable& variable : scope->variables)
    if (variable.type_id)
      variable.type = simgrid::util::find_map_ptr(
        info->types, variable.type_id);

  // Recursive post-processing of nested-scopes:
  for (simgrid::mc::Frame& nested_scope : scope->scopes)
      mc_post_process_scope(info, &nested_scope);

}

/** \brief Fill/lookup the "subtype" field.
 */
static void MC_resolve_subtype(simgrid::mc::ObjectInformation* info, simgrid::mc::Type* type)
{
  if (!type->type_id)
    return;
  type->subtype = simgrid::util::find_map_ptr(info->types, type->type_id);
  if (type->subtype == nullptr)
    return;
  if (type->subtype->byte_size != 0)
    return;
  if (type->subtype->name.empty())
    return;
  // Try to find a more complete description of the type:
  // We need to fix in order to support C++.
  simgrid::mc::Type** subtype = simgrid::util::find_map_ptr(
    info->full_types_by_name, type->subtype->name);
  if (subtype)
    type->subtype = *subtype;
}

static void MC_post_process_types(simgrid::mc::ObjectInformation* info)
{
  // Lookup "subtype" field:
  for(auto& i : info->types) {
    MC_resolve_subtype(info, &(i.second));
    for (simgrid::mc::Type& member : i.second.members)
      MC_resolve_subtype(info, &member);
  }
}

/** \brief Finds informations about a given shared object/executable */
std::shared_ptr<simgrid::mc::ObjectInformation> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char *name, int executable)
{
  std::shared_ptr<simgrid::mc::ObjectInformation> result =
    std::make_shared<simgrid::mc::ObjectInformation>();
  if (executable)
    result->flags |= simgrid::mc::ObjectInformation::Executable;
  result->file_name = name;
  MC_find_object_address(maps, result.get());
  MC_dwarf_get_variables(result.get());
  MC_post_process_variables(result.get());
  MC_post_process_types(result.get());
  for (auto& entry : result.get()->subprograms)
    mc_post_process_scope(result.get(), &entry.second);
  MC_make_functions_index(result.get());
  return std::move(result);
}

/*************************************************************************/

void MC_post_process_object_info(simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info)
{
  for (auto& i : info->types) {

    simgrid::mc::Type* type = &(i.second);
    simgrid::mc::Type* subtype = type;
    while (subtype->type == DW_TAG_typedef
        || subtype->type == DW_TAG_volatile_type
        || subtype->type == DW_TAG_const_type)
      if (subtype->subtype)
        subtype = subtype->subtype;
      else
        break;

    // Resolve full_type:
    if (!subtype->name.empty() && subtype->byte_size == 0) {
      for (auto const& object_info : process->object_infos) {
        auto i = object_info->full_types_by_name.find(subtype->name);
        if (i != object_info->full_types_by_name.end()
            && !i->second->name.empty() && i->second->byte_size) {
          type->full_type = i->second;
          break;
        }
      }
    } else type->full_type = subtype;

  }
}
