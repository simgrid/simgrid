/* datadesc - data description in order to send/recv it in XBT             */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "datadesc_private.h"
#include "portable.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ddt, xbt, "Data description");
/* FIXME: make this host-dependent using a trick such as UserData*/
/*@null@*/ xbt_set_t xbt_datadesc_set_local = NULL;


/* callback for array size when sending strings */
static int _strlen_cb( /*@unused@ */ xbt_datadesc_type_t type, /*@unused@ */
                      xbt_cbps_t vars, void *data)
{

  return 1 + (int) strlen(data);
}


/**
 * xbt_datadesc_init:
 *
 * Initialize the datadesc module.
 * FIXME: We assume that when neither signed nor unsigned is given,
 *    that means signed. To be checked by configure.
 **/
void xbt_datadesc_preinit(void)
{
  xbt_datadesc_type_t ddt;     /* What to add */

  /* only initialize once */
  if (xbt_datadesc_set_local != NULL)
    return;

  XBT_VERB("Initializing DataDesc");

  xbt_datadesc_set_local = xbt_set_new();


  /* all known datatypes */

  ddt = xbt_datadesc_scalar("signed char",
                             xbt_ddt_scalar_char,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("char",
                             xbt_ddt_scalar_char,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("unsigned char",
                             xbt_ddt_scalar_char,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("signed short int",
                             xbt_ddt_scalar_short,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("short int",
                             xbt_ddt_scalar_short,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("short",
                             xbt_ddt_scalar_short,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("unsigned short int",
                             xbt_ddt_scalar_short,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("signed int",
                             xbt_ddt_scalar_int,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("int",
                             xbt_ddt_scalar_int,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("unsigned int",
                             xbt_ddt_scalar_int,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("signed long int",
                             xbt_ddt_scalar_long,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("long int",
                             xbt_ddt_scalar_long,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("long",
                             xbt_ddt_scalar_long,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("unsigned long int",
                             xbt_ddt_scalar_long,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("signed long long int",
                             xbt_ddt_scalar_long_long,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("long long int",
                             xbt_ddt_scalar_long_long,
                             e_xbt_dd_scalar_encoding_sint);
  ddt = xbt_datadesc_scalar("unsigned long long int",
                             xbt_ddt_scalar_long_long,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("data pointer",
                             xbt_ddt_scalar_pdata,
                             e_xbt_dd_scalar_encoding_uint);
  ddt = xbt_datadesc_scalar("function pointer",
                             xbt_ddt_scalar_pfunc,
                             e_xbt_dd_scalar_encoding_uint);

  ddt = xbt_datadesc_scalar("float",
                             xbt_ddt_scalar_float,
                             e_xbt_dd_scalar_encoding_float);
  ddt = xbt_datadesc_scalar("double",
                             xbt_ddt_scalar_double,
                             e_xbt_dd_scalar_encoding_float);

  ddt = xbt_datadesc_array_dyn("char[]",
                                xbt_datadesc_by_name("char"), _strlen_cb);
  xbt_datadesc_ref("string", ddt);
  xbt_datadesc_ref("xbt_string_t", ddt);

  /* specific datatype: the exception type (for RPC) */
  ddt = xbt_datadesc_struct("ex_t");
  xbt_datadesc_struct_append(ddt, "msg", xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "category",
                              xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(ddt, "value", xbt_datadesc_by_name("int"));

  xbt_datadesc_struct_append(ddt, "remote",
                              xbt_datadesc_by_name("short int"));

  xbt_datadesc_struct_append(ddt, "host",
                              xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "procname",
                              xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "pid",
                              xbt_datadesc_by_name("long int"));
  xbt_datadesc_struct_append(ddt, "file",
                              xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "line", xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_append(ddt, "func",
                              xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "used", xbt_datadesc_by_name("int"));
  xbt_datadesc_cb_field_push(ddt, "used");
  xbt_datadesc_struct_append(ddt, "bt_strings",
                              xbt_datadesc_ref_pop_arr
                              (xbt_datadesc_by_name("string")));

  xbt_datadesc_struct_close(ddt);

  /* specific datatype: xbt_peer_t */
  ddt = xbt_datadesc_struct("s_xbt_peer_t");
  xbt_datadesc_struct_append(ddt, "name",
                              xbt_datadesc_by_name("string"));
  xbt_datadesc_struct_append(ddt, "port", xbt_datadesc_by_name("int"));
  xbt_datadesc_struct_close(ddt);

  ddt = xbt_datadesc_ref("xbt_peer_t", ddt);

  /* Dict containing the constant value (for the parsing macro) */
  xbt_dd_constants = xbt_dict_new_homogeneous(xbt_free_f);

}

/**
 * xbt_datadesc_exit:
 *
 * Finalize the datadesc module
 **/
void xbt_datadesc_postexit(void)
{
  XBT_VERB("Exiting DataDesc");
  xbt_set_free(&xbt_datadesc_set_local);
  xbt_dict_free(&xbt_dd_constants);
  XBT_DEBUG("Exited DataDesc");
}

/** This is mainly to debug */
const char *xbt_datadesc_get_name(xbt_datadesc_type_t ddt)
{
  return ddt ? (const char *) ddt->name : "(null)";
}

/** This is mainly to debug */
int xbt_datadesc_get_id(xbt_datadesc_type_t ddt)
{
  return ddt->code;
}

/**
 * xbt_datadesc_size:
 *
 * Returns the size occuped by data of this type (on the current arch).
 *
 */
int xbt_datadesc_size(xbt_datadesc_type_t type)
{
  return type ? type->size[GRAS_THISARCH] : 0;
}

/**
 * xbt_datadesc_type_dump:
 *
 * For debugging purpose
 */
void xbt_datadesc_type_dump(const xbt_datadesc_type_t ddt)
{
  unsigned int cpt;

  printf("DataDesc dump:");
  if (!ddt) {
    printf("(null)\n");
    return;
  }
  printf("%s (ID:%d)\n", ddt->name, ddt->code);
  printf("  category: %s\n", xbt_datadesc_cat_names[ddt->category_code]);

  printf("  size[");
  for (cpt = 0; cpt < xbt_arch_count; cpt++) {
    printf("%s%s%ld%s",
           cpt > 0 ? ", " : "",
           cpt == GRAS_THISARCH ? ">" : "",
           ddt->size[cpt], cpt == GRAS_THISARCH ? "<" : "");
  }
  printf("]\n");

  printf("  alignment[");
  for (cpt = 0; cpt < xbt_arch_count; cpt++) {
    printf("%s%s%lu%s",
           cpt > 0 ? ", " : "",
           cpt == GRAS_THISARCH ? ">" : "",
           ddt->alignment[cpt], cpt == GRAS_THISARCH ? "<" : "");
  }
  printf("]\n");

  printf("  aligned_size[");
  for (cpt = 0; cpt < xbt_arch_count; cpt++) {
    printf("%s%s%lu%s",
           cpt > 0 ? ", " : "",
           cpt == GRAS_THISARCH ? ">" : "",
           ddt->aligned_size[cpt], cpt == GRAS_THISARCH ? "<" : "");
  }
  printf("]\n");
  if (ddt->category_code == e_xbt_datadesc_type_cat_struct) {
    xbt_dd_cat_struct_t struct_data;
    xbt_dd_cat_field_t field;

    struct_data = ddt->category.struct_data;
    xbt_dynar_foreach(struct_data.fields, cpt, field) {
      printf(">>> Dump field #%u (%s) (offset=%ld)\n",
             cpt, field->name, field->offset[GRAS_THISARCH]);
      xbt_datadesc_type_dump(field->type);
      printf("<<< end dump field #%u (%s)\n", cpt, field->name);
    }
  }
  fflush(stdout);
}
