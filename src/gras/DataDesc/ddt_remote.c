/* $Id$ */

/* ddt_remote - Stuff needed to get datadescs about remote hosts            */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"


/***
 *** Table of all known architectures. 
 ***/

const gras_arch_sizes_t gras_arch_sizes[gras_arch_count] = {
  {"i386",   0,   {1,2,4,4,8,   4,4,   4,8}}
};

/**
 * gras_free_ddt:
 *
 * gime that memory back, dude. I mean it.
 */
static void gras_free_ddt(void *ddt) {
  gras_datadesc_type_t *type= (gras_datadesc_type_t *)ddt;
  
  if (type) {
    gras_ddt_free(&type);
  }
}

/**
 * gras_ddt_register:
 *
 * Add a type to a type set
 */
gras_error_t gras_ddt_register(gras_datadesc_type_t *type) {
  return gras_set_add(gras_datadesc_set_local,
		      (gras_set_elm_t*)type,
                       &gras_free_ddt);

}

/**
 * gras_datadesc_by_name:
 *
 * Retrieve a type from its name
 */
gras_error_t gras_datadesc_by_name(const char            *name,
				   gras_datadesc_type_t **type) {
  return gras_set_get_by_name(gras_datadesc_set_local,
			      name,(gras_set_elm_t**)type);
}

/**
 * gras_datadesc_by_id:
 *
 * Retrieve a type from its code
 */
gras_error_t gras_datadesc_by_id(long int               code,
				 gras_datadesc_type_t **type) {
  return gras_set_get_by_id(gras_datadesc_set_local,
			    code,(gras_set_elm_t**)type);
}

/**
 * gras_dd_convert_elm:
 *
 * Convert the element described by @type comming from architecture @r_arch.
 * The data to be converted is stored in @src, and is to be stored in @dst.
 * Both pointers may be the same location if no resizing is needed.
 */
gras_error_t
gras_dd_convert_elm(gras_datadesc_type_t *type,
		    int r_arch, 
		    void *src, void *dst) {

  if (r_arch != GRAS_THISARCH) 
    RAISE_UNIMPLEMENTED;

  return no_error;
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
