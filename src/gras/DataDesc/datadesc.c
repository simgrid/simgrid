/* $Id$ */

/* datadesc - data description in order to send/recv it in GRAS             */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(DataDesc,GRAS);
/* FIXME: make this host-dependent using a trick such as UserData*/
gras_set_t *gras_datadesc_set_local=NULL;


/* callback for array size when sending strings */
static int
_strlen_cb(void			*vars,
           gras_datadesc_type_t	*type,
           void			*data) {

  return 1+(long int)strlen(data);
}


/**
 * gras_datadesc_init:
 *
 * Initialize the datadesc module.
 * FIXME: We assume that when neither signed nor unsigned is given, 
 *    that means signed. To be checked by configure.
 **/
void
gras_datadesc_init(void) {
  gras_error_t errcode;
  gras_datadesc_type_t *ddt; /* What to add */
  gras_datadesc_type_t *elm; /* element of ddt when needed */

  /* only initialize once */
  if (gras_datadesc_set_local != NULL)
    return;
  
  VERB0("Initializing DataDesc");
  
  TRYFAIL(gras_set_new(&gras_datadesc_set_local));
  
  TRYFAIL(gras_datadesc_declare_scalar("signed char", 
				       gras_ddt_scalar_char, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("char", 
				       gras_ddt_scalar_char, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("unsigned char", 
				       gras_ddt_scalar_char, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("signed short int", 
				       gras_ddt_scalar_short, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("short int", 
				       gras_ddt_scalar_short, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("unsigned short int", 
				       gras_ddt_scalar_short, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("signed int", 
				       gras_ddt_scalar_int, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("int", 
				       gras_ddt_scalar_int, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("unsigned int", 
				       gras_ddt_scalar_int, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("signed long int", 
				       gras_ddt_scalar_long, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("long int", 
				       gras_ddt_scalar_long, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("unsigned long int", 
				       gras_ddt_scalar_long, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("signed long long int", 
				       gras_ddt_scalar_long_long, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("long long int", 
				       gras_ddt_scalar_long_long, 
				       e_gras_dd_scalar_encoding_sint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("unsigned long long int", 
				       gras_ddt_scalar_long_long, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("data pointer", 
				       gras_ddt_scalar_pdata, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("function pointer", 
				       gras_ddt_scalar_pfunc, 
				       e_gras_dd_scalar_encoding_uint, 
				       NULL, &ddt));
  
  TRYFAIL(gras_datadesc_declare_scalar("float", 
				       gras_ddt_scalar_float, 
				       e_gras_dd_scalar_encoding_float, 
				       NULL, &ddt));
  TRYFAIL(gras_datadesc_declare_scalar("double", 
				       gras_ddt_scalar_float, 
				       e_gras_dd_scalar_encoding_float, 
				       NULL,&ddt));

  TRYFAIL(gras_datadesc_declare_array_dyn("string", 
					  gras_datadesc_by_name("char"), 
					  _strlen_cb,&ddt));

  //  elm=ddt;
  //  TRYFAIL(gras_datadesc_declare_ref("string", elm, &ddt));
}

/**
 * gras_datadesc_exit:
 *
 * Finalize the datadesc module
 **/
void
gras_datadesc_exit(void) {
  VERB0("Exiting DataDesc");
  gras_set_free(&gras_datadesc_set_local);
  gras_datadesc_set_local = NULL;
}


