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
 */
static uint64_t MC_dwarf_default_lower_bound(int lang);

static uint64_t MC_dwarf_subrange_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** \brief Computes the number of elements of a given DW_TAG_array_type
 *
 */
static uint64_t MC_dwarf_array_element_count(Dwarf_Die* die, Dwarf_Die* unit);

/** \brief Checks if a given tag is a (known) type tag.
 */
static int MC_dwarf_tag_type(int tag);
static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit);
static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit);
static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit);
static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit);
static Dwarf_Die* MC_dwarf_resolve_die(Dwarf_Die* die, int attribute);

const char* MC_dwarf_attrname(int attr) {
  switch (attr) {
#include "mc_dwarf_attrnames.h"
  default:
    return "DW_AT_unkown";
  }
}

const char* MC_dwarf_tagname(int tag) {
  switch (tag) {
#include "mc_dwarf_tagnames.h"
  case DW_TAG_invalid:
    return "DW_TAG_invalid";
  default:
    return "DW_TAG_unkown";
  }
}

static inline const char* MC_dwarf_die_tagname(Dwarf_Die* die) {
  return MC_dwarf_tagname(dwarf_tag(die));
}

static int MC_dwarf_tag_type(int tag) {
  switch(tag) {
  case DW_TAG_array_type:
  case DW_TAG_class_type:
  case DW_TAG_enumeration_type:
  case DW_TAG_typedef:
  case DW_TAG_pointer_type:
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
  case DW_TAG_rvalue_reference_type:
    return 1;
  default:
    return 0;
  }
}

static const char* MC_dwarf_attr_string(Dwarf_Die* die, int attribute) {
  Dwarf_Attribute attr;
  if (!dwarf_attr_integrate(die, attribute, &attr)) {
	return NULL;
  } else {
	return dwarf_formstring(&attr);
  }
}

// Return a new string for the type (NULL if none)
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

static uint64_t MC_dwarf_attr_uint(Dwarf_Die* die, int attribute, uint64_t default_value) {
  Dwarf_Attribute attr;
  Dwarf_Word value;
  return dwarf_formudata(dwarf_attr_integrate(die, attribute, &attr), &value) == 0 ? (uint64_t) value : default_value;
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
  // Use DW_TAG_count if present:
  if (dwarf_hasattr_integrate(die, DW_AT_count)) {
    return MC_dwarf_attr_uint(die, DW_AT_count, 0);
  }

  // Otherwise compute DW_TAG_upper_bound-DW_TAG_lower_bound:

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
  return upper_bound - lower_bound;
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
  switch (dwarf_whatform(&attr)) {

  case DW_FORM_exprloc:
    {
      //TODO, location can be an integer as well
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
        xbt_die("Can't groke this location expression yet. %i %i",
          len==1 , expr[0].atom == DW_OP_plus_uconst);
      }
      break;
    }
  case DW_FORM_data1:
  case DW_FORM_data2:
  case DW_FORM_data4:
  case DW_FORM_data8:
  case DW_FORM_sdata:
  case DW_FORM_udata:
    {
      Dwarf_Word offset;
      if (!dwarf_formudata(&attr, &offset))
        member->offset = offset;
      else
        xbt_die("Cannot get DW_AT_data_member_%s location <%p>%s",
          MC_dwarf_attr_string(child, DW_AT_name),
          type->id, type->name);
      break;
    }
  }
}

static void MC_dwarf_add_members(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit, dw_type_t type) {
  int res;
  Dwarf_Die child;
  xbt_assert(!type->members);
  type->members = xbt_dynar_new(sizeof(dw_type_t), (void(*)(void*))dw_type_free);
  for (res=dwarf_child(die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
    if (dwarf_tag(&child)==DW_TAG_member) {
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

      xbt_dynar_push(type->members, &member);
    }
  }
}

static dw_type_t MC_dwarf_die_to_type(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit) {

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
  }

  return type;
}

static void MC_dwarf_handle_type_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit) {
  dw_type_t type = MC_dwarf_die_to_type(info, die, unit);

  char* key = bprintf("%" PRIx64, (uint64_t) type->id);
  xbt_dict_set(info->types, key, type, NULL);
}


static void MC_dwarf_handle_children(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit) {
  Dwarf_Die child;
  int res;
  for (res=dwarf_child(die, &child); res==0; res=dwarf_siblingof(&child,&child)) {
    MC_dwarf_handle_die(info, &child, unit);
  }
}

static void MC_dwarf_handle_die(mc_object_info_t info, Dwarf_Die* die, Dwarf_Die* unit) {
	int tag = dwarf_tag(die);

	// Register types:
	if (MC_dwarf_tag_type(tag))
		MC_dwarf_handle_type_die(info, die, unit);

	// Other tag-specific handling:
	else switch(tag) {

	}

	// Recursive processing od children DIE:
	MC_dwarf_handle_children(info, die, unit);
}

void MC_dwarf_get_variables_libdw(mc_object_info_t info) {
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
    Dwarf_Die die;
    if(dwarf_offdie(dwarf, offset+length, &die)!=NULL) {
      MC_dwarf_handle_die(info, &die, &die);
    }
    offset = next_offset;
  }

  dwarf_end(dwarf);
  close(fd);
}
