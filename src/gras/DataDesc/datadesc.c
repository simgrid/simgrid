/* $Id$ */

/* datadesc - data description in order to send/recv it in GRAS             */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(datadesc,GRAS);
/* FIXME: make this host-dependent using a trick such as UserData*/
gras_set_t *gras_datadesc_set_local=NULL;


/* callback for array size when sending strings */
static int
_strlen_cb(gras_cbps_t *vars, void *data) {

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

  /* only initialize once */
  if (gras_datadesc_set_local != NULL)
    return;
  
  VERB0("Initializing DataDesc");
  
  TRYFAIL(gras_set_new(&gras_datadesc_set_local));
  
  TRYFAIL(gras_datadesc_scalar("signed char", 
			       gras_ddt_scalar_char, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("char", 
			       gras_ddt_scalar_char, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("unsigned char", 
			       gras_ddt_scalar_char, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("signed short int", 
			       gras_ddt_scalar_short, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("short int", 
			       gras_ddt_scalar_short, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("unsigned short int", 
			       gras_ddt_scalar_short, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("signed int", 
			       gras_ddt_scalar_int, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("int", 
			       gras_ddt_scalar_int, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("unsigned int", 
			       gras_ddt_scalar_int, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("signed long int", 
			       gras_ddt_scalar_long, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("long int", 
			       gras_ddt_scalar_long, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("unsigned long int", 
			       gras_ddt_scalar_long, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("signed long long int", 
			       gras_ddt_scalar_long_long, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("long long int", 
			       gras_ddt_scalar_long_long, 
			       e_gras_dd_scalar_encoding_sint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("unsigned long long int", 
			       gras_ddt_scalar_long_long, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("data pointer", 
			       gras_ddt_scalar_pdata, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("function pointer", 
			       gras_ddt_scalar_pfunc, 
			       e_gras_dd_scalar_encoding_uint, 
			       &ddt));
  
  TRYFAIL(gras_datadesc_scalar("float", 
			       gras_ddt_scalar_float, 
			       e_gras_dd_scalar_encoding_float, 
			       &ddt));
  TRYFAIL(gras_datadesc_scalar("double", 
			       gras_ddt_scalar_double, 
			       e_gras_dd_scalar_encoding_float, 
			       &ddt));

  TRYFAIL(gras_datadesc_array_dyn("char[]", 
				  gras_datadesc_by_name("char"), 
				  _strlen_cb,&ddt));
  TRYFAIL(gras_datadesc_ref("string", 
			    gras_datadesc_by_name("char[]"), 
			    &ddt));

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
  DEBUG0("Exited DataDesc");
}

/**
 * gras_datadesc_get_name:
 *
 * Returns the name of a datadescription (to ease the debug)
 */
char *
gras_datadesc_get_name(gras_datadesc_type_t *ddt) {
  return ddt->name;
}
/**
 * gras_datadesc_get_id:
 *
 * Returns the name of a datadescription (to ease the debug)
 */
int
gras_datadesc_get_id(gras_datadesc_type_t *ddt) {
  return ddt->code;
}

/**
 * gras_datadesc_size: 
 *
 * Returns the size occuped by data of this type (on the current arch).
 *
 */
int gras_datadesc_size(gras_datadesc_type_t *type) {
  return type->size[GRAS_THISARCH];
}

/**
 * gras_datadesc_type_dump:
 *
 * For debugging purpose
 */
void gras_datadesc_type_dump(const gras_datadesc_type_t *ddt){
  int cpt;

  printf("DataDesc dump:");
  if(!ddt) {
    printf("(null)\n");
    return;
  }
  printf ("%s (ID:%d)\n",ddt->name,ddt->code);
  printf ("  category: %s\n",gras_datadesc_cat_names[ddt->category_code]);

  printf ("  size[");
  for (cpt=0; cpt<gras_arch_count; cpt++) {
    printf("%s%s%ld%s",
	   cpt>0?", ":"",
	   cpt == GRAS_THISARCH ? ">":"",
	   ddt->size[cpt],
	   cpt == GRAS_THISARCH ? "<":"");
  }
  printf ("]\n");

  printf ("  alignment[");
  for (cpt=0; cpt<gras_arch_count; cpt++) {
    printf("%s%s%ld%s",
	   cpt>0?", ":"",
	   cpt == GRAS_THISARCH ? ">":"",
	   ddt->alignment[cpt],
	   cpt == GRAS_THISARCH ? "<":"");
  }
  printf ("]\n");

  printf ("  aligned_size[");
  for (cpt=0; cpt<gras_arch_count; cpt++) {
    printf("%s%s%ld%s",
	   cpt>0?", ":"",
	   cpt == GRAS_THISARCH ? ">":"",
	   ddt->aligned_size[cpt],
	   cpt == GRAS_THISARCH ? "<":"");
  }
  printf ("]\n");
  if (ddt->category_code == e_gras_datadesc_type_cat_struct) {
    gras_dd_cat_struct_t  struct_data;
    gras_dd_cat_field_t  *field;

    struct_data = ddt->category.struct_data;
    gras_dynar_foreach(struct_data.fields, cpt, field) {
    printf(">>> Dump field #%d (%s) (offset=%ld)\n",
	   cpt,field->name,field->offset[GRAS_THISARCH]);
    gras_datadesc_type_dump(field->type);
    printf("<<< end dump field #%d (%s)\n",cpt,field->name);
    }
 }
  fflush(stdout);
}
