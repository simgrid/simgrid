/* $Id$ */

/* ddt_remote - Stuff needed to get datadescs about remote hosts            */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

/* callback for array size when sending strings */
static int
_strlen_cb(void			*vars,
           gras_datadesc_type_t	*type,
           void			*data) {

  //  (void)p_vars; /* FIXME(Oli): Why that? Avoid warning? That's not needed*/
  //  (void)p_type;

  return 1+(long int)strlen(data);
}

/***
 *** Table of all known architectures. 
 ***/

typedef struct {
  const char *name;

  int endian;

  int sizeof_char;
  int sizeof_short;
  int sizeof_int;
  int sizeof_long;
  int sizeof_long_long;

  int sizeof_pdata;
  int sizeof_pfunc;

  int sizeof_float;
  int sizeof_double;
} gras_arch_sizes_t;

const gras_arch_sizes_t gras_arch_sizes[] = {
  {"i386",   0,   1,2,4,4,8,   4,4,   4,8}
};

const int gras_arch_sizes_count = 1;

/**
 * gras_free_ddt:
 *
 * gime that memory back, dude
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
gras_error_t gras_ddt_register(gras_set_t           *set,
			       gras_datadesc_type_t *type) {
  return gras_set_add(set,
		      (gras_set_elm_t*)type,
                       &gras_free_ddt);

}

/**
 * gras_ddt_get_by_name:
 *
 * Retrieve a type from its name
 */
gras_error_t gras_ddt_get_by_name(gras_set_t            *set,
				  const char            *name,
				  gras_datadesc_type_t **type) {
  return gras_set_get_by_name(set,name,(gras_set_elm_t**)type);
}

/**
 * gras_dd_typeset_create:
 *
 * create a type set, and bootstrap it by declaring all basic types in it
 */
gras_error_t
gras_dd_typeset_create(int gras_arch,
		       gras_set_t **s) {
  gras_error_t errcode;
  gras_datadesc_type_t *ddt; /* What to add */
  gras_datadesc_type_t *elm; /* element of ddt when needed */
  gras_set_t *set; /* result */

  if (gras_arch >= gras_arch_sizes_count) {
    RAISE1(mismatch_error, "Remote architecture signature (=%d) unknown locally\n", gras_arch);
  }

  TRY(gras_set_new(s));
  set=*s;

  TRY(gras_ddt_new_scalar("signed char", 
			  gras_arch_sizes[gras_arch].sizeof_char,
			  e_gras_dd_scalar_encoding_sint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("unsigned char", 
			  gras_arch_sizes[gras_arch].sizeof_char,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("signed short int", 
			  gras_arch_sizes[gras_arch].sizeof_short,
			  e_gras_dd_scalar_encoding_sint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("unsigned short int", 
			  gras_arch_sizes[gras_arch].sizeof_short,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("signed int", 
			  gras_arch_sizes[gras_arch].sizeof_int,
			  e_gras_dd_scalar_encoding_sint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("unsigned int", 
			  gras_arch_sizes[gras_arch].sizeof_int,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("signed long int", 
			  gras_arch_sizes[gras_arch].sizeof_long,
			  e_gras_dd_scalar_encoding_sint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("unsigned long int", 
			  gras_arch_sizes[gras_arch].sizeof_long,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("signed long long int", 
			  gras_arch_sizes[gras_arch].sizeof_long_long,
			  e_gras_dd_scalar_encoding_sint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("unsigned long long int", 
			  gras_arch_sizes[gras_arch].sizeof_long_long,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("data pointer", 
			  gras_arch_sizes[gras_arch].sizeof_pdata,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("function pointer", 
			  gras_arch_sizes[gras_arch].sizeof_pfunc,
			  e_gras_dd_scalar_encoding_uint, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));


  TRY(gras_ddt_new_scalar("float", 
			  gras_arch_sizes[gras_arch].sizeof_float,
			  e_gras_dd_scalar_encoding_float, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_new_scalar("double", 
			  gras_arch_sizes[gras_arch].sizeof_float,
			  e_gras_dd_scalar_encoding_float, 
			  NULL,
			  &ddt));
  TRY(gras_ddt_register(set,ddt));

  TRY(gras_ddt_get_by_name(set,"unsigned char",&elm));
  TRY(gras_ddt_new_array("string", elm, 0, _strlen_cb, NULL, &ddt));
  TRY(gras_ddt_register(set,ddt));
			 

  return no_error;
}

