/* $Id$ */

/* ddt_remote - Stuff needed to get datadescs about remote hosts            */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

/************************************************************************/
/* C combines the power of assembler with the portability of assembler. */
/************************************************************************/

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(convert,DataDesc);

/***
 *** Table of all known architectures. 
 ***/

const gras_arch_desc_t gras_arches[gras_arch_count] = {
  {"i386",   0,   {1,2,4,4,8,   4,4,   4,8}},
  {"alpha",  0,   {1,2,4,8,8,   8,8,   4.8}},
  {"powerpc",1,   {1,2,4,4,8,   4,4,   4,8}},
  {"sparc",  1,   {1,2,4,8,8,   8,8,   4,8}}
};

const char *gras_datadesc_arch_name(int code) {
   if (code < 0 || code >= gras_arch_count)
     return "[unknown arch]";
   return gras_arches[code].name;
}

/**
 * gras_dd_convert_elm:
 *
 * Convert the element described by @type comming from architecture @r_arch.
 * The data to be converted is stored in @src, and is to be stored in @dst.
 * Both pointers may be the same location if no resizing is needed.
 */
gras_error_t
gras_dd_convert_elm(gras_datadesc_type_t *type, int count,
		    int r_arch, 
		    void *src, void *dst) {
  //  gras_dd_cat_scalar_t scal_data = type->category.scal_data;

  gras_assert(type->category_code == e_gras_datadesc_type_cat_scalar);

  if (r_arch == GRAS_THISARCH)
    return no_error;

  if (gras_arches[r_arch].endian == gras_arches[GRAS_THISARCH].endian &&
      type->aligned_size[r_arch] == type->aligned_size[GRAS_THISARCH])
    return no_error;

  

  RAISE_UNIMPLEMENTED;
}

/**
 * gras_arch_selfid:
 *
 * returns the ID of the architecture the process is running on
 */
int
gras_arch_selfid(void) {
  return GRAS_THISARCH;
}
