/* $Id$ */

/* datadesc - describing the data to exchange                               */

/* module's private interface masked even to other parts of GRAS.           */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_PRIVATE_H
#define GRAS_DATADESC_PRIVATE_H

#include "gras_private.h"
#include "gras/DataDesc/datadesc_interface.h"

/**
 * aligned:
 * 
 * Align the data v on the boundary a.
 */
#define aligned(v, a) (((v) + (a - 1)) & ~(a - 1))

extern gras_set_t *gras_datadesc_set_local;
void gras_ddt_freev(void *ddt);
/*******************************************
 * Descriptions of all known architectures *
 *******************************************/

#define gras_arch_count 4
typedef enum {
  gras_ddt_scalar_char      = 0,
  gras_ddt_scalar_short     = 1,
  gras_ddt_scalar_int       = 2,
  gras_ddt_scalar_long      = 3,
  gras_ddt_scalar_long_long = 4,

  gras_ddt_scalar_pdata     = 5,
  gras_ddt_scalar_pfunc     = 6,

  gras_ddt_scalar_float     = 7,
  gras_ddt_scalar_double    = 8
} gras_ddt_scalar_type_t;

typedef struct {
  const char *name;

  int endian;

  int sizeofs[9]; /* char,short,int,long,long_long,
		   pdata,pfunc,
		   float,double */
  int boundaries[9]; /* idem */
} gras_arch_desc_t;

extern const gras_arch_desc_t gras_arches[gras_arch_count];
extern const char *gras_datadesc_cat_names[9];

/**********************************************************/
/* Actual definitions of the stuff in the type descriptor */
/**********************************************************/

/**
 * e_gras_datadesc_type_category:
 *
 * Defines all possible type categories. 
 */
typedef enum e_gras_datadesc_type_category {
  
  /* if you edit this, also fix gras_datadesc_cat_names in ddt_exchange.c */

  e_gras_datadesc_type_cat_undefined = 0,
  
  e_gras_datadesc_type_cat_scalar = 1,
  e_gras_datadesc_type_cat_struct = 2,
  e_gras_datadesc_type_cat_union = 3,
  e_gras_datadesc_type_cat_ref = 4,       /* ref to an uniq element */
  e_gras_datadesc_type_cat_array = 5,
  e_gras_datadesc_type_cat_ignored = 6,
  
  e_gras_datadesc_type_cat_invalid = 7
} gras_datadesc_type_category_t;

/*------------------------------------------------*/
/* definitions of specific data for each category */
/*------------------------------------------------*/
/**
 * s_gras_dd_cat_field:
 *
 * Fields of struct and union
 */
typedef struct s_gras_dd_cat_field {

  char 	   *name;
  long int  offset[gras_arch_count];
  gras_datadesc_type_t *type;
  
  gras_datadesc_type_cb_void_t pre;
  gras_datadesc_type_cb_void_t post;

} gras_dd_cat_field_t;

void gras_dd_cat_field_free(void *f);

/**
 * gras_dd_cat_scalar_t:
 *
 * Specific fields of a scalar
 */
enum e_gras_dd_scalar_encoding {
  e_gras_dd_scalar_encoding_undefined = 0,
  
  e_gras_dd_scalar_encoding_uint,
  e_gras_dd_scalar_encoding_sint,
  e_gras_dd_scalar_encoding_float,
  
  e_gras_dd_scalar_encoding_invalid 
};
typedef struct s_gras_dd_cat_scalar {
  enum e_gras_dd_scalar_encoding encoding;
  gras_ddt_scalar_type_t type; /* to check easily that redefinition matches*/
} gras_dd_cat_scalar_t;

/**
 * gras_dd_cat_struct_t:
 *
 * Specific fields of a struct
 */
typedef struct s_gras_dd_cat_struct {
  gras_dynar_t *fields; /* elm type = gras_dd_cat_field_t */
  int closed; /* gras_datadesc_declare_struct_close() was called */
} gras_dd_cat_struct_t;

/**
 * gras_dd_cat_union_t:
 *
 * Specific fields of a union
 */
typedef struct s_gras_dd_cat_union {
  gras_datadesc_type_cb_int_t selector;
  gras_dynar_t *fields; /* elm type = gras_dd_cat_field_t */
  int closed; /* gras_datadesc_declare_union_close() was called */
} gras_dd_cat_union_t;

/**
 * gras_dd_cat_ref_t:
 *
 * Specific fields of a reference
 */
typedef struct s_gras_dd_cat_ref {
  gras_datadesc_type_t     *type;

  /* callback used to return the referenced type number  */
  gras_datadesc_selector_t  selector;
} gras_dd_cat_ref_t;


/**
 * gras_dd_cat_array_t:
 *
 * Specific fields of an array
 */
typedef struct s_gras_dd_cat_array {
  gras_datadesc_type_t *type;

  /* element_count < 0 means dynamically defined */
  long int                  	  fixed_size;

  /* callback used to return the dynamic length */
  gras_datadesc_type_cb_int_t dynamic_size;

} gras_dd_cat_array_t;

/**
 * gras_dd_cat_ignored_t:
 *
 * Specific fields of an ignored field
 */
typedef struct s_gras_dd_cat_ignored {
  void	 	 		*default_value;
  void_f_pvoid_t                *free_func;
} gras_dd_cat_ignored_t;


/**
 * u_gras_datadesc_category:
 *
 * Specific data to each possible category
 */
union u_gras_datadesc_category {
        void                  *undefined_data;
        gras_dd_cat_scalar_t   scalar_data;
        gras_dd_cat_struct_t   struct_data;
        gras_dd_cat_union_t    union_data;
        gras_dd_cat_ref_t      ref_data;
        gras_dd_cat_array_t    array_data;
        gras_dd_cat_ignored_t  ignored_data;
};


/****************************************/
/* The holy grail: type descriptor type */
/****************************************/
/**
 * s_gras_datadesc_type:
 *
 * Type descriptor.
 */
struct s_gras_datadesc_type {
  /* headers for the data set */
  unsigned int                         code;
  char                                *name;
  unsigned int                         name_len;
        
  /* payload */
  long int                             size[gras_arch_count];
  
  long int                             alignment[gras_arch_count];  
  long int                             aligned_size[gras_arch_count];
  
  enum  e_gras_datadesc_type_category  category_code;
  union u_gras_datadesc_category       category;
  
  gras_datadesc_type_cb_void_t         send;
  gras_datadesc_type_cb_void_t         recv;
};

/***************************
 * constructor/desctructor *
 ***************************/
void gras_datadesc_free(gras_datadesc_type_t *type);

gras_error_t 
gras_datadesc_scalar(const char                       *name,
		     gras_ddt_scalar_type_t           type,
		     enum e_gras_dd_scalar_encoding   encoding,
		     gras_datadesc_type_t           **dst);

/****************************************************
 * Callback persistant state constructor/destructor *
 ****************************************************/
gras_error_t
gras_cbps_new(gras_cbps_t **dst);
void
gras_cbps_free(gras_cbps_t **state);

/***************
 * Convertions *
 ***************/
gras_error_t
gras_dd_convert_elm(gras_datadesc_type_t *type, int count,
		    int r_arch, 
		    void *src, void *dst);

#endif /* GRAS_DATADESC_PRIVATE_H */

