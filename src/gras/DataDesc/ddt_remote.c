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
 * gras_ddt_get_by_name:
 *
 * Retrieve a type from its name
 */
gras_error_t gras_ddt_get_by_name(const char            *name,
				  gras_datadesc_type_t **type) {
  return gras_set_get_by_name(gras_datadesc_set_local,name,(gras_set_elm_t**)type);
}

/**
 * gras_ddt_get_by_code:
 *
 * Retrieve a type from its name
 */
gras_error_t gras_ddt_get_by_code(int                    code,
				  gras_datadesc_type_t **type) {
  return gras_set_get_by_id(gras_datadesc_set_local,code,(gras_set_elm_t**)type);
}
