/* datadesc - describing the data to exchange                               */

/* module's private interface masked even to other parts of XBT.           */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_DATADESC_PRIVATE_H
#define XBT_DATADESC_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/set.h"

#include "gras_config.h"        /* XBT_THISARCH */
#include "xbt_modinter.h"       /* module init/exit */
#include "datadesc_interface.h" /* semi-public API */

/**
 * ddt_aligned:
 *
 * Align the data v on the boundary a.
 */
#define ddt_aligned(v, a) (((v) + (a - 1)) & ~(a - 1))

/*@null@*/ extern xbt_set_t xbt_datadesc_set_local;
void xbt_ddt_freev(void *ddt);
/*******************************************
 * Descriptions of all known architectures *
 *******************************************/

#define xbt_arch_count 12
typedef enum {
  xbt_ddt_scalar_char = 0,
  xbt_ddt_scalar_short = 1,
  xbt_ddt_scalar_int = 2,
  xbt_ddt_scalar_long = 3,
  xbt_ddt_scalar_long_long = 4,

  xbt_ddt_scalar_pdata = 5,
  xbt_ddt_scalar_pfunc = 6,

  xbt_ddt_scalar_float = 7,
  xbt_ddt_scalar_double = 8
} xbt_ddt_scalar_type_t;

typedef struct {
  const char *name;

  int endian;

  int sizeofs[9];               /* char,short,int,long,long_long,pdata,pfunc,float,double */
  int boundaries[9];            /* idem */
} xbt_arch_desc_t;

extern const xbt_arch_desc_t xbt_arches[xbt_arch_count];
extern const char *xbt_datadesc_cat_names[9];

/**********************************************************/
/* Actual definitions of the stuff in the type descriptor */
/**********************************************************/

/**
 * e_xbt_datadesc_type_category:
 *
 * Defines all possible type categories.
 */
typedef enum e_xbt_datadesc_type_category {

  /* if you edit this, also fix xbt_datadesc_cat_names in ddt_exchange.c */

  e_xbt_datadesc_type_cat_undefined = 0,

  e_xbt_datadesc_type_cat_scalar = 1,
  e_xbt_datadesc_type_cat_struct = 2,
  e_xbt_datadesc_type_cat_union = 3,
  e_xbt_datadesc_type_cat_ref = 4,     /* ref to an uniq element */
  e_xbt_datadesc_type_cat_array = 5,

  e_xbt_datadesc_type_cat_invalid = 6
} xbt_datadesc_type_category_t;

/*------------------------------------------------*/
/* definitions of specific data for each category */
/*------------------------------------------------*/
/**
 * s_xbt_dd_cat_field:
 *
 * Fields of struct and union
 */
typedef struct s_xbt_dd_cat_field {

  char *name;
  long int offset[xbt_arch_count];
  xbt_datadesc_type_t type;

  xbt_datadesc_type_cb_void_t send;
  xbt_datadesc_type_cb_void_t recv;

} s_xbt_dd_cat_field_t, *xbt_dd_cat_field_t;

void xbt_dd_cat_field_free(void *f);

/**
 * xbt_dd_cat_scalar_t:
 *
 * Specific fields of a scalar
 */
enum e_xbt_dd_scalar_encoding {
  e_xbt_dd_scalar_encoding_undefined = 0,

  e_xbt_dd_scalar_encoding_uint,
  e_xbt_dd_scalar_encoding_sint,
  e_xbt_dd_scalar_encoding_float,

  e_xbt_dd_scalar_encoding_invalid
};
typedef struct s_xbt_dd_cat_scalar {
  enum e_xbt_dd_scalar_encoding encoding;
  xbt_ddt_scalar_type_t type;  /* to check easily that redefinition matches */
} xbt_dd_cat_scalar_t;

/**
 * xbt_dd_cat_struct_t:
 *
 * Specific fields of a struct
 */
typedef struct s_xbt_dd_cat_struct {
  xbt_dynar_t fields;           /* elm type = xbt_dd_cat_field_t */
  int closed;                   /* xbt_datadesc_declare_struct_close() was called */
} xbt_dd_cat_struct_t;

/**
 * xbt_dd_cat_union_t:
 *
 * Specific fields of a union
 */
typedef struct s_xbt_dd_cat_union {
  xbt_datadesc_type_cb_int_t selector;
  xbt_dynar_t fields;           /* elm type = xbt_dd_cat_field_t */
  int closed;                   /* xbt_datadesc_declare_union_close() was called */
} xbt_dd_cat_union_t;

/**
 * xbt_dd_cat_ref_t:
 *
 * Specific fields of a reference
 */
typedef struct s_xbt_dd_cat_ref {
  xbt_datadesc_type_t type;

  /* callback used to return the referenced type number  */
  xbt_datadesc_selector_t selector;
} xbt_dd_cat_ref_t;


/**
 * xbt_dd_cat_array_t:
 *
 * Specific fields of an array
 */
typedef struct s_xbt_dd_cat_array {
  xbt_datadesc_type_t type;

  /* element_count == -1 means dynamically defined */
  long int fixed_size;

  /* callback used to return the dynamic length */
  xbt_datadesc_type_cb_int_t dynamic_size;

} xbt_dd_cat_array_t;

/**
 * u_xbt_datadesc_category:
 *
 * Specific data to each possible category
 */
union u_xbt_datadesc_category {
  void *undefined_data;
  xbt_dd_cat_scalar_t scalar_data;
  xbt_dd_cat_struct_t struct_data;
  xbt_dd_cat_union_t union_data;
  xbt_dd_cat_ref_t ref_data;
  xbt_dd_cat_array_t array_data;
};

/****************************************/
/* The holy grail: type descriptor type */
/****************************************/
/**
 * s_xbt_datadesc_type:
 *
 * Type descriptor.
 */
typedef struct s_xbt_datadesc_type {
  /* headers for the data set */
  int code;
  char *name;
  unsigned int name_len;

  /* payload */
  long int size[xbt_arch_count];       /* Cannot be unsigned: -1 means dynamic */

  unsigned long int alignment[xbt_arch_count];
  unsigned long int aligned_size[xbt_arch_count];

  enum e_xbt_datadesc_type_category category_code;
  union u_xbt_datadesc_category category;

  xbt_datadesc_type_cb_void_t send;
  xbt_datadesc_type_cb_void_t recv;

  /* flags */
  int cycle:1;

  /* random value for users (like default value or whatever) */
  char extra[SIZEOF_MAX];

} s_xbt_datadesc_type_t;

/***************************
 * constructor/desctructor *
 ***************************/
void xbt_datadesc_free(xbt_datadesc_type_t * type);

xbt_datadesc_type_t
xbt_datadesc_scalar(const char *name,
                     xbt_ddt_scalar_type_t type,
                     enum e_xbt_dd_scalar_encoding encoding);

/****************************************************
 * Callback persistant state constructor/destructor *
 ****************************************************/
xbt_cbps_t xbt_cbps_new(void);
void xbt_cbps_free(xbt_cbps_t * state);
void xbt_cbps_reset(xbt_cbps_t state);

/***************
 * Convertions *
 ***************/
void
xbt_dd_convert_elm(xbt_datadesc_type_t type, int count,
                    int r_arch, void *src, void *dst);

/********************************************************************
 * Dictionnary containing the constant values for the parsing macro *
 ********************************************************************/
extern xbt_dict_t xbt_dd_constants;    /* lives in ddt_parse.c of course */

#endif                          /* XBT_DATADESC_PRIVATE_H */
