/* Copyright (c) 2008-2013. The SimGrid Team.
 * All rights reserved.                                                     */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <dwarf.h>
#include <elfutils/libdw.h>
#include <inttypes.h>

#include <simgrid_config.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "mc_private.h"

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
static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** \brief Computes the number of elements of a given DW_TAG_array_type.
 *
 * \param die DIE for the DW_TAG_array_type
 */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** \brief Process a DIE
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace);

/** \brief Process a type DIE
 */
static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace);

/** \brief Calls MC_dwarf_handle_die on all childrend of the given die
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace);

/** \brief Handle a variable (DW_TAG_variable or other)
 *
 *  \param info the resulting object fot the library/binary file (output)
 *  \param die  the current DIE
 *  \param unit the DIE of the compile unit of the current DIE
 *  \param frame containg frame if any
 */
static void MC_dwarf_handle_variable_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace);

/** \brief Convert a libdw DWARF expression into a MC representation of the location
 *
 *  \param expr array of DWARf operations
 *  \param len  number of elements
 *  \return a new MC expression
 */
static dw_location_t MC_dwarf_get_expression(Dwarf_Op* expr,  size_t len);

/** \brief Get the DW_TAG_type of the DIE
 *
 *  \param die DIE
 *  \return DW_TAG_type attribute as a new string (NULL if none)
 */
static char* MC_dwarf_at_type(Dwarf_Die* die);

/** \brief Get the name of an attribute (DW_AT_*) from its code
 *
 *  \param attr attribute code (see the DWARF specification)
 *  \return name of the attribute
 */
const char* MC_dwarf_attrname(int attr) {
  switch (attr) {
#include "mc_dwarf_attrnames.h"
  default:
    return "DW_AT_unknown";
  }
}

/** \brief Get the name of a dwarf tag (DW_TAG_*) from its code
 *
 *  \param tag tag code (see the DWARF specification)
 *  \return name of the tag
 */
const char* MC_dwarf_tagname(int tag) {
  switch (tag) {
#include "mc_dwarf_tagnames.h"
  case DW_TAG_invalid:
    return "DW_TAG_invalid";
  default:
    return "DW_TAG_unknown";
  }
}

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

static mc_tag_class MC_dwarf_tag_classify(int tag) {
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
    case DW_TAG_mutable_type:
    case DW_TAG_shared_type:
      return mc_tag_type;

    case DW_TAG_subprogram:
      return mc_tag_subprogram;

    case DW_TAG_variable:
    case DW_TAG_formal_parameter:
      return mc_tag_variable;

    case DW_TAG_lexical_block:
    case DW_TAG_try_block:
    case DW_TAG_inlined_subroutine:
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

static int MC_dwarf_form_get_class(int form) {
  switch(form) {
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
static inline const char* MC_dwarf_die_tagname(Dwarf_Die* die) {
  return MC_dwarf_tagname(dwarf_tag(die));
}

// ***** Attributes

/** \brief Get an attribute of a given DIE as a string
 *
 *  \param the DIE
 *  \param attribute attribute
 *  \return value of the given attribute of the given DIE
 */
static const char* MC_dwarf_attr_string(Dwarf_Die* die, int attribute) {
  Dwarf_Attribute attr;
  if (!dwarf_attr_integrate(die, attribute, &attr)) {
	return NULL;
  } else {
	return dwarf_formstring(&attr);
  }
}

/** \brief Get the linkage name of a DIE.
 *
 *  Use either DW_AT_linkage_name or DW_AR_MIPS_linkage_name.
 *
 *  \param DIE
 *  \return linkage name of the given DIE (or NULL)
 * */
static const char* MC_dwarf_at_linkage_name(Dwarf_Die* die) {
  const char* name = MC_dwarf_attr_string(die, DW_AT_linkage_name);
  if (!name)
    name = MC_dwarf_attr_string(die, DW_AT_MIPS_linkage_name);
  return name;
}

/** \brief Create a location list from a given attribute
 *
 *  \param die the DIE
 *  \param attr the attribute
 *  \return MC specific representation of the location list represented by the given attribute
 *  of the given die
 */
static dw_location_t MC_dwarf_get_location_list(mc_object_info_t info, Dwarf_Die* die, Dwarf_Attribute* attr) {

  dw_location_t location = xbt_new0(s_dw_location_t, 1);
  location->type = e_dw_loclist;
  xbt_dynar_t loclist = xbt_dynar_new(sizeof(dw_location_entry_t), NULL);
  location->location.loclist = loclist;

  ptrdiff_t offset = 0;
  Dwarf_Addr base, start, end;
  Dwarf_Op *expr;
  size_t len;

  while (1) {

    offset = dwarf_getlocations(attr, offset, &base, &start, &end, &expr, &len);
    if (offset==0)
      return location;
    else if (offset==-1)
      xbt_die("Error while loading location list");

    dw_location_entry_t new_entry = xbt_new0(s_dw_location_entry_t, 1);

    void* base = info->flags & MC_OBJECT_INFO_EXECUTABLE ? 0 : MC_object_base_address(info);

    new_entry->lowpc = (char*) base + start;
    new_entry->highpc = (char*) base + end;
    new_entry->location = MC_dwarf_get_expression(expr, len);

    xbt_dynar_push(loclist, &new_entry);

  }
}

/** \brief Find the frame base of a given frame
 *
 *  \param ip         Instruction pointer
 *  \param frame
 *  \param unw_cursor
 */
void* mc_find_frame_base(void* ip, dw_frame_t frame, unw_cursor_t* unw_cursor) {
  switch(frame->frame_base->type) {
  case e_dw_loclist:
  {
    int loclist_cursor;
    for(loclist_cursor=0; loclist_cursor < xbt_dynar_length(frame->frame_base->location.loclist); loclist_cursor++){
      dw_location_entry_t entry = xbt_dynar_get_as(frame->frame_base->location.loclist, loclist_cursor, dw_location_entry_t);
      if((ip >= entry->lowpc) && (ip < entry->highpc)){
        return (void*) MC_dwarf_resolve_location(unw_cursor, entry->location, NULL);
      }
    }
    return NULL;
  }
  // Not handled:
  default:
    return NULL;
  }
}

/** \brief Get the location expression or location list from an attribute
 *
 *  Processes direct expressions as well as location lists.
 *
 *  \param die the DIE
 *  \param attr the attribute
 *  \return MC specific representation of the location represented by the given attribute
 *  of the given die
 */
static dw_location_t MC_dwarf_get_location(mc_object_info_t info, Dwarf_Die* die, Dwarf_Attribute* attr) {
  int form = dwarf_whatform(attr);
  switch (form) {

  // The attribute is an DWARF location expression:
  case DW_FORM_exprloc:
  case DW_FORM_block1: // not in the spec
  case DW_FORM_block2:
  case DW_FORM_block4:
  case DW_FORM_block:
    {
      Dwarf_Op* expr;
      size_t len;
      if (dwarf_getlocation(attr, &expr, &len))
        xbt_die("Could not read location expression");
      return MC_dwarf_get_expression(expr, len);
    }

  // The attribute is a reference to a location list entry:
  case DW_FORM_sec_offset:
  case DW_FORM_data1:
  case DW_FORM_data2:
  case DW_FORM_data4:
  case DW_FORM_data8:
    {
      return MC_dwarf_get_location_list(info, die, attr);
    }
    break;

  default:
    xbt_die("Unexpected form %i list for location in attribute %s of <%p>%s",
      form,
      MC_dwarf_attrname(attr->code),
      (void*) dwarf_dieoffset(die),
      MC_dwarf_attr_string(die, DW_AT_name));
    return NULL;
  }
}

/** \brief Get the location expression or location list from an attribute
 *
 *  Processes direct expressions as well as location lists.
 *
 *  \param die the DIE
 *  \param attribute the attribute code
 *  \return MC specific representation of the location represented by the given attribute
 *  of the given die
 */
static dw_location_t MC_dwarf_at_location(mc_object_info_t info, Dwarf_Die* die, int attribute) {
  if(!dwarf_hasattr_integrate(die, attribute))
    return xbt_new0(s_dw_location_t, 1);

  Dwarf_Attribute attr;
  dwarf_attr_integrate(die, attribute, &attr);
  return MC_dwarf_get_location(info, die, &attr);
}

static char* MC_dwarf_at_type(Dwarf_Die* die) {
  Dwarf_Attribute attr;
  if (dwarf_hasattr_integrate(die, DW_AT_type)) {
	dwarf_attr_integrate(die, DW_AT_type, &attr);
	Dwarf_Die subtype_die;
	if (dwarf_formref_die(&attr, &subtype_die)==NULL) {
	  xbt_die("Could not find DIE for type");
	}
	Dwarf_Off subtype_global_offset = dwarf_dieoffset(&subtype_die);
    return bprintf("%" PRIx64 , subtype_global_offset);
  }
  else return NULL;
}

static uint64_t MC_dwarf_attr_addr(Dwarf_Die* die, int attribute) {
  Dwarf_Attribute attr;
  if(dwarf_attr_integrate(die, attribute, &attr)==NULL)
    return 0;
  Dwarf_Addr value;
  if (dwarf_formaddr(&attr, &value) == 0)
    return (uint64_t) value;
  else
    return 0;
}

static uint64_t MC_dwarf_attr_uint(Dwarf_Die* die, int attribute, uint64_t default_value) {
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate(die, attribute, &attr)==NULL)
    return default_value;
  Dwarf_Word value;
  return dwarf_formudata(dwarf_attr_integrate(die, attribute, &attr), &value) == 0 ? (uint64_t) value : default_value;
}

static bool MC_dwarf_attr_flag(Dwarf_Die* die, int attribute, int integrate) {
  Dwarf_Attribute attr;
  if ((integrate ? dwarf_attr_integrate(die, attribute, &attr)
                    : dwarf_attr(die, attribute, &attr))==0)
    return false;

  bool result;
  if (dwarf_formflag(&attr, &result))
    xbt_die("Unexpected form for attribute %s",
      MC_dwarf_attrname(attribute));
  return result;
}

static uint64_t MC_dwarf_default_lower_bound(int lang) {
  switch(lang) {
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
    xbt_die("No default MT_TAG_lower_bound for language %i and none given", lang);
    return 0;
  }
}

static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die* die, Dwarf_Die* unit) {
  xbt_assert(dwarf_tag(die)==DW_TAG_enumeration_type ||dwarf_tag(die)==DW_TAG_subrange_type,
      "MC_dwarf_subrange_element_count called with DIE of type %s", MC_dwarf_die_tagname(die));

  // Use DW_TAG_count if present:
  if (dwarf_hasattr_integrate(die, DW_AT_count)) {
    return MC_dwarf_attr_uint(die, DW_AT_count, 0);
  }

  // Otherwise compute DW_TAG_upper_bound-DW_TAG_lower_bound + 1:

  if (!dwarf_hasattr_integrate(die, DW_AT_upper_bound)) {
	// This is not really 0, but the code expects this (we do not know):
    return 0;
  }
  uint64_t upper_bound = MC_dwarf_attr_uint(die, DW_AT_upper_bound, -1);

  uint64_t lower_bound = 0;
  if (dwarf_hasattr_integrate(die, DW_AT_lower_bound)) {
    lower_bound = MC_dwarf_attr_uint(die, DW_AT_lower_bound, -1);
  } else {
	lower_bound = MC_dwarf_default_lower_bound(dwarf_srclang(unit));
  }
  return upper_bound - lower_bound + 1;
}

static uint64_t MC_dwarf_array_element_count(Dwarf_Die* die, Dwarf_Die* unit) {
  xbt_assert(dwarf_tag(die)==DW_TAG_array_type,
    "MC_dwarf_array_element_count called with DIE of type %s", MC_dwarf_die_tagname(die));

  int result = 1;
  Dwarf_Die child;
  int res;
  for (res=dwarf_child(die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
	int child_tag = dwarf_tag(&child);
    if (child_tag==DW_TAG_subrange_type ||child_tag==DW_TAG_enumeration_type) {
      result *= MC_dwarf_subrange_element_count(&child, unit);
    }
  }
  return result;
}

// ***** Location

Dwarf_Off MC_dwarf_resolve_location(unw_cursor_t* c, dw_location_t location, void* frame_pointer_address) {
  unw_word_t res;
  switch (location->type){
  case e_dw_compose:
    if (xbt_dynar_length(location->location.compose) > 1){
      return 0; /* TODO : location list with optimizations enabled */
    }
    dw_location_t location_entry = xbt_dynar_get_as(location->location.compose, 0, dw_location_t);
    switch (location_entry->type){
    case e_dw_register:
      unw_get_reg(c, location_entry->location.reg, &res);
      return res;
    case e_dw_bregister_op:
      unw_get_reg(c, location_entry->location.breg_op.reg, &res);
      return (Dwarf_Off) ((long)res + location_entry->location.breg_op.offset);
      break;
    case e_dw_fbregister_op:
      if (frame_pointer_address != NULL)
        return (Dwarf_Off)((char *)frame_pointer_address + location_entry->location.fbreg_op);
      else
        return 0;
    default:
      return 0; /* FIXME : implement other cases (with optimizations enabled) */
    }
    break;
    default:
      return 0;
  }
}

// ***** dw_type_t

static void MC_dwarf_fill_member_location(dw_type_t type, dw_type_t member, Dwarf_Die* child) {
  if (dwarf_hasattr(child, DW_AT_data_bit_offset)) {
    xbt_die("Can't groke DW_AT_data_bit_offset.");
  }

  if (!dwarf_hasattr_integrate(child, DW_AT_data_member_location)) {
    if (type->type != DW_TAG_union_type) {
        xbt_die(
          "Missing DW_AT_data_member_location field in DW_TAG_member %s of type <%p>%s",
          member->name, type->id, type->name);
    } else {
      return;
    }
  }

  Dwarf_Attribute attr;
  dwarf_attr_integrate(child, DW_AT_data_member_location, &attr);
  int form  = dwarf_whatform(&attr);
  int klass = MC_dwarf_form_get_class(form);
  switch (klass) {
  case MC_DW_CLASS_EXPRLOC:
  case MC_DW_CLASS_BLOCK:
    // Location expression:
    {
      Dwarf_Op* expr;
      size_t len;
      if (dwarf_getlocation(&attr, &expr, &len)) {
        xbt_die(
          "Could not read location expression DW_AT_data_member_location in DW_TAG_member %s of type <%p>%s",
          MC_dwarf_attr_string(child, DW_AT_name),
          type->id, type->name);
      }
      if (len==1 && expr[0].atom == DW_OP_plus_uconst) {
        member->offset =  expr[0].number;
      } else {
        xbt_die("Can't groke this location expression yet.");
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
        xbt_die("Cannot get %s location <%p>%s",
          MC_dwarf_attr_string(child, DW_AT_name),
          type->id, type->name);
      break;
    }
  case MC_DW_CLASS_LOCLISTPTR:
    // Reference to a location list:
    // TODO
  case MC_DW_CLASS_REFERENCE:
    // It's supposed to be possible in DWARF2 but I couldn't find its semantic
    // in the spec.
  default:
    xbt_die(
      "Can't handle form class (%i) / form 0x%x as DW_AT_member_location",
      klass, form);
  }

}

static void MC_dwarf_add_members(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_type_t type) {
  int res;
  Dwarf_Die child;
  xbt_assert(!type->members);
  type->members = xbt_dynar_new(sizeof(dw_type_t), (void(*)(void*))dw_type_free);
  for (res=dwarf_child(die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
    if (dwarf_tag(&child)==DW_TAG_member) {

      // Skip declarations:
      if (MC_dwarf_attr_flag(&child, DW_AT_declaration, false))
        continue;

      // Skip compile time constants:
      if(dwarf_hasattr(&child, DW_AT_const_value))
        continue;

      // TODO, we should use another type (because is is not a type but a member)
      dw_type_t member = xbt_new0(s_dw_type_t, 1);
      member->type = -1;
      member->id = NULL;

      const char* name = MC_dwarf_attr_string(&child, DW_AT_name);
      if(name)
        member->name = xbt_strdup(name);
      else
        member->name = NULL;

      member->byte_size = MC_dwarf_attr_uint(&child, DW_AT_byte_size, 0);
      member->element_count = -1;
      member->dw_type_id = MC_dwarf_at_type(&child);
      member->members = NULL;
      member->is_pointer_type = 0;
      member->offset = 0;

      if(dwarf_hasattr(&child, DW_AT_data_bit_offset)) {
        xbt_die("Can't groke DW_AT_data_bit_offset.");
      }

      MC_dwarf_fill_member_location(type, member, &child);

      if (!member->dw_type_id) {
        xbt_die("Missing type for member %s of <%p>%s", member->name, type->id, type->name);
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
static dw_type_t MC_dwarf_die_to_type(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {

  dw_type_t type = xbt_new0(s_dw_type_t, 1);
  type->type = -1;
  type->id = NULL;
  type->name = NULL;
  type->byte_size = 0;
  type->element_count = -1;
  type->dw_type_id = NULL;
  type->members = NULL;
  type->is_pointer_type = 0;
  type->offset = 0;

  type->type = dwarf_tag(die);

  // Global Offset
  type->id = (void *) dwarf_dieoffset(die);

  const char* name = MC_dwarf_attr_string(die, DW_AT_name);
  if (name!=NULL) {
	type->name = xbt_strdup(name);
  }

  XBT_DEBUG("Processing type <%p>%s", type->id, type->name);

  type->dw_type_id = MC_dwarf_at_type(die);

  // Computation of the byte_size;
  if (dwarf_hasattr_integrate(die, DW_AT_byte_size))
    type->byte_size = MC_dwarf_attr_uint(die, DW_AT_byte_size, 0);
  else if (type->type == DW_TAG_array_type || type->type==DW_TAG_structure_type || type->type==DW_TAG_class_type) {
    Dwarf_Word size;
    if (dwarf_aggregate_size(die, &size)==0) {
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
	  char* new_namespace = namespace == NULL ? xbt_strdup(type->name)
	    : bprintf("%s::%s", namespace, type->name);
	  MC_dwarf_handle_children(info, die, unit, frame, new_namespace);
	  free(new_namespace);
	  break;
  }

  return type;
}

static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  dw_type_t type = MC_dwarf_die_to_type(info, die, unit, frame, namespace);

  char* key = bprintf("%" PRIx64, (uint64_t) type->id);
  xbt_dict_set(info->types, key, type, NULL);

  if(type->name && type->byte_size!=0) {
    xbt_dict_set(info->types_by_name, type->name, type, NULL);
  }
}

/** \brief Convert libdw location expresion elment into native one (or NULL in some cases) */
static dw_location_t MC_dwarf_get_expression_element(Dwarf_Op* op) {
  dw_location_t element = xbt_new0(s_dw_location_t, 1);
  uint8_t atom = op->atom;
  if (atom >= DW_OP_reg0 && atom<= DW_OP_reg31) {
    element->type = e_dw_register;
    element->location.reg = atom - DW_OP_reg0;
  }
  else if (atom >= DW_OP_breg0 && atom<= DW_OP_breg31) {
    element->type = e_dw_bregister_op;
    element->location.reg = atom - DW_OP_breg0;
    element->location.breg_op.offset = op->number;
  }
  else if (atom >= DW_OP_lit0 && atom<= DW_OP_lit31) {
    element->type = e_dw_lit;
    element->location.reg = atom - DW_OP_lit0;
  }
  else switch (atom) {
  case DW_OP_fbreg:
    element->type = e_dw_fbregister_op;
    element->location.fbreg_op = op->number;
    break;
  case DW_OP_piece:
    element->type = e_dw_piece;
    element->location.piece = op->number;
    break;
  case DW_OP_plus_uconst:
    element->type = e_dw_plus_uconst;
    element->location.plus_uconst = op->number;
    break;
  case DW_OP_abs:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("abs");
    break;
  case DW_OP_and:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("and");
    break;
  case DW_OP_div:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("div");
    break;
  case DW_OP_minus:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("minus");
    break;
  case DW_OP_mod:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("mod");
    break;
  case DW_OP_mul:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("mul");
    break;
  case DW_OP_neg:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("neg");
    break;
  case DW_OP_not:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("not");
    break;
  case DW_OP_or:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("or");
    break;
  case DW_OP_plus:
    element->type = e_dw_arithmetic;
    element->location.arithmetic = xbt_strdup("plus");
    break;

  case DW_OP_stack_value:
    // Why nothing here?
    xbt_free(element);
    return NULL;

  case DW_OP_deref_size:
    element->type = e_dw_deref;
    element->location.deref_size =  (unsigned int short) op->number;
    break;
  case DW_OP_deref:
    element->type = e_dw_deref;
    element->location.deref_size = sizeof(void *);
    break;
  case DW_OP_constu:
    element->type = e_dw_uconstant;
    element->location.uconstant.bytes = 1;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_consts:
    element->type = e_dw_sconstant;
    element->location.uconstant.bytes = 1;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;

  case DW_OP_const1u:
    element->type = e_dw_uconstant;
    element->location.uconstant.bytes = 1;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const2u:
    element->type = e_dw_uconstant;
    element->location.uconstant.bytes = 2;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const4u:
    element->type = e_dw_uconstant;
    element->location.uconstant.bytes = 4;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const8u:
    element->type = e_dw_uconstant;
    element->location.uconstant.bytes = 8;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;

  case DW_OP_const1s:
    element->type = e_dw_sconstant;
    element->location.uconstant.bytes = 1;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const2s:
    element->type = e_dw_sconstant;
    element->location.uconstant.bytes = 2;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const4s:
    element->type = e_dw_sconstant;
    element->location.uconstant.bytes = 4;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  case DW_OP_const8s:
    element->type = e_dw_sconstant;
    element->location.uconstant.bytes = 8;
    element->location.uconstant.value = (unsigned long int) op->number;
    break;
  default:
    element->type = e_dw_unsupported;
    break;
  }
  return element;
}

/** \brief Convert libdw location expresion into native one */
static dw_location_t MC_dwarf_get_expression(Dwarf_Op* expr,  size_t len) {
  dw_location_t loc = xbt_new0(s_dw_location_t, 1);
  loc->type = e_dw_compose;
  loc->location.compose = xbt_dynar_new(sizeof(dw_location_t), NULL);

  int i;
  for (i=0; i!=len; ++i) {
    dw_location_t element =  MC_dwarf_get_expression_element(expr+i);
    if (element)
      xbt_dynar_push(loc->location.compose, &element);
  }

  return loc;
}

static int mc_anonymous_variable_index = 0;

static dw_variable_t MC_die_to_variable(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  // Skip declarations:
  if (MC_dwarf_attr_flag(die, DW_AT_declaration, false))
    return NULL;

  // Skip compile time constants:
  if(dwarf_hasattr(die, DW_AT_const_value))
    return NULL;

  Dwarf_Attribute attr_location;
  if (dwarf_attr(die, DW_AT_location, &attr_location)==NULL) {
    // No location: do not add it ?
    return NULL;
  }

  dw_variable_t variable = xbt_new0(s_dw_variable_t, 1);
  variable->dwarf_offset = dwarf_dieoffset(die);
  variable->global = frame == NULL; // Can be override base on DW_AT_location

  const char* name = MC_dwarf_attr_string(die, DW_AT_name);
  variable->name = xbt_strdup(name);

  variable->type_origin = MC_dwarf_at_type(die);

  int klass = MC_dwarf_form_get_class(dwarf_whatform(&attr_location));
  switch (klass) {
  case MC_DW_CLASS_EXPRLOC:
  case MC_DW_CLASS_BLOCK:
    // Location expression:
    {
      Dwarf_Op* expr;
      size_t len;
      if (dwarf_getlocation(&attr_location, &expr, &len)) {
        xbt_die(
          "Could not read location expression in DW_AT_location of variable <%p>%s",
          (void*) variable->dwarf_offset, variable->name);
      }

      if (len==1 && expr[0].atom == DW_OP_addr) {
        variable->global = 1;
        Dwarf_Off offset = expr[0].number;
        // TODO, Why is this different base on the object?
        Dwarf_Off base = strcmp(info->file_name, xbt_binary_name) !=0 ? (Dwarf_Off) info->start_exec : 0;
        variable->address = (void*) (base + offset);
      } else {
        variable->location = MC_dwarf_get_expression(expr, len);
      }

      break;
    }
  case MC_DW_CLASS_LOCLISTPTR:
  case MC_DW_CLASS_CONSTANT:
    // Reference to location list:
    variable->location = MC_dwarf_get_location_list(info, die, &attr_location);
    break;
  default:
    xbt_die("Unexpected calss 0x%x (%i) list for location in <%p>%s",
      klass, klass, (void*) variable->dwarf_offset, variable->name);
  }

  // Handle start_scope:
  if (dwarf_hasattr(die, DW_AT_start_scope)) {
    Dwarf_Attribute attr;
    dwarf_attr(die, DW_AT_start_scope, &attr);
    int form  = dwarf_whatform(&attr);
    int klass = MC_dwarf_form_get_class(form);
    switch(klass) {
    case MC_DW_CLASS_CONSTANT:
    {
      Dwarf_Word value;
      variable->start_scope = dwarf_formudata(&attr, &value) == 0 ? (size_t) value : 0;
      break;
    }
    default:
      xbt_die("Unhandled form 0x%x, class 0x%X for DW_AT_start_scope of variable %s",
        form, klass, name==NULL ? "?" : name);
    }
  }

  if(namespace && variable->global) {
    char* old_name = variable->name;
    variable->name = bprintf("%s::%s", namespace, old_name);
    free(old_name);
  }

  // The current code needs a variable name,
  // generate a fake one:
  if(!variable->name) {
    variable->name = bprintf("@anonymous#%i", mc_anonymous_variable_index++);
  }

  return variable;
}

static void MC_dwarf_handle_variable_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  dw_variable_t variable = MC_die_to_variable(info, die, unit, frame, namespace);
  if(variable==NULL)
      return;
  MC_dwarf_register_variable(info, frame, variable);
}

static void MC_dwarf_handle_subprogram_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t parent_frame, const char* namespace) {
  dw_frame_t frame = xbt_new0(s_dw_frame_t, 1);

  frame->start = dwarf_dieoffset(die);

  const char* name = MC_dwarf_attr_string(die, DW_AT_name);
  frame->name = namespace ? bprintf("%s::%s", namespace, name) : xbt_strdup(name);

  // This is the base address for DWARF addresses.
  // Relocated addresses are offset from this base address.
  // See DWARF4 spec 7.5
  void* base = info->flags & MC_OBJECT_INFO_EXECUTABLE ? 0 : MC_object_base_address(info);

  // Variables are filled in the (recursive) call of MC_dwarf_handle_children:
  frame->variables = xbt_dynar_new(sizeof(dw_variable_t), dw_variable_free_voidp);
  frame->high_pc = ((char*) base) + MC_dwarf_attr_addr(die, DW_AT_high_pc);
  frame->low_pc = ((char*) base) + MC_dwarf_attr_addr(die, DW_AT_low_pc);
  frame->frame_base = MC_dwarf_at_location(info, die, DW_AT_frame_base);
  frame->end = -1; // This one is now useless:

  // Register it:
  xbt_dynar_push(info->subprograms, &frame);

  // Handle children:
  MC_dwarf_handle_children(info, die, unit, frame, namespace);
}

static void mc_dwarf_handle_namespace_die(
    mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  const char* name = MC_dwarf_attr_string(die, DW_AT_name);
  if(frame)
    xbt_die("Unexpected namespace in a subprogram");
  char* new_namespace = namespace == NULL ? xbt_strdup(name)
    : bprintf("%s::%s", namespace, name);
  MC_dwarf_handle_children(info, die, unit, frame, new_namespace);
}

static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  Dwarf_Die child;
  int res;
  for (res=dwarf_child(die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
    MC_dwarf_handle_die(info, &child, unit, frame, namespace);
  }
}

static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_frame_t frame, const char* namespace) {
  int tag = dwarf_tag(die);
  mc_tag_class klass = MC_dwarf_tag_classify(tag);
  switch (klass) {

    // Type:
    case mc_tag_type:
      MC_dwarf_handle_type_die(info, die, unit, frame, namespace);
      break;

    // Program:
    case mc_tag_subprogram:
      MC_dwarf_handle_subprogram_die(info, die, unit, frame, namespace);
      return;

    // Variable:
    case mc_tag_variable:
      MC_dwarf_handle_variable_die(info, die, unit, frame, namespace);
      break;

    // Scope:
    case mc_tag_scope:
      // TODO
      break;

    case mc_tag_namespace:
      mc_dwarf_handle_namespace_die(info, die, unit, frame, namespace);
      break;

    default:
      break;

  }
}

void MC_dwarf_get_variables(mc_object_info_t info) {
  int fd = open(info->file_name, O_RDONLY);
  if (fd<0) {
    xbt_die("Could not open file %s", info->file_name);
  }
  Dwarf *dwarf = dwarf_begin(fd, DWARF_C_READ);
  if (dwarf==NULL) {
    xbt_die("Your program must be compiled with -g");
  }

  Dwarf_Off offset = 0;
  Dwarf_Off next_offset = 0;
  size_t length;
  while (dwarf_nextcu (dwarf, offset, &next_offset, &length, NULL, NULL, NULL) == 0) {
    Dwarf_Die unit_die;

    if(dwarf_offdie(dwarf, offset+length, &unit_die)!=NULL) {
      Dwarf_Die child;
      int res;
      for (res=dwarf_child(&unit_die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
        MC_dwarf_handle_die(info, &child, &unit_die, NULL, NULL);
      }
    }
    offset = next_offset;
  }

  dwarf_end(dwarf);
  close(fd);
}
