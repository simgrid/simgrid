/* $Id$ */

/* ddt_new - creation/deletion of datatypes structs (private to this module)*/

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(new,DataDesc);

static gras_error_t
gras_ddt_new(const char            *name,
	     gras_datadesc_type_t **dst) {

  gras_datadesc_type_t *res=malloc(sizeof(gras_datadesc_type_t));
  if (!res) 
    RAISE_MALLOC;

  memset(res, 0, sizeof(res));
  res->name = strdup(name);
  res->name_len = strlen(name);
  res->pre = NULL;
  res->post = NULL;
  
  *dst=res;
  return no_error;
}

/**
 * gras_ddt_new_scalar:
 *
 * Create a new scalar and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_scalar(const char                      *name,
		    gras_ddt_scalar_type_t           type,
		    enum e_gras_dd_scalar_encoding   encoding,
		    gras_datadesc_type_cb_void_t     cb,
		    gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  long int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch = 0; arch < gras_arch_count; arch ++) {
    long int sz;
    long int mask;

    res->size[arch] = gras_arch_sizes[arch].sizeof_scalars[type];
    
    sz = res->size[arch];
    mask = sz;
    
    /* just in case you wonder, x>>1 == x/2 on all architectures when x>=0 and a size is always>=0 */

    /* make sure mask have all the bits under the biggest one of size set to 1
       Example: size=000100101 => mask=0000111111 */
    while ((sz >>= 1)) {
      mask |= sz;
    }
    
    if (res->size[arch] & (mask >> 1)) { /* if size have bits to one beside its biggest */
      /* size is not a power of 2 */
      /* alignment= next power of 2 after size */
      res->alignment[arch] = (res->size[arch] & ~(mask >> 1)) << 1;
      gras_assert0(res->alignment[arch] != 0,
		   "scalar type too large");
      
      res->aligned_size[arch]    = aligned(res->size[arch], res->alignment[arch]);
      gras_assert0 (res->aligned_size[arch] >= 0,
		    "scalar type too large");
      
    } else {
      /* size is a power of 2, life is great */
      res->alignment[arch]       = res->size[arch];
      res->aligned_size[arch]    = res->size[arch];
    }

    /* FIXME size < 0 sometimes? 
       } else {
       res->alignment    = 0;
       res->aligned_size = 0;
       }
    */
  }

  res->category_code                 = e_gras_datadesc_type_cat_scalar;
  res->category.scalar_data.encoding = encoding;

  res->pre = cb;
  return no_error;
}


/**
 * gras_dd_cat_field_free:
 *
 * Frees one struct or union field
 */
void gras_dd_cat_field_free(void *f) {
  gras_dd_cat_field_t *field = (gras_dd_cat_field_t *)f;
  if (field) {
    if (field->name) 
      free(field->name);
    free(field);
  }
}

/**
 * gras_ddt_new_struct:
 *
 * Create a new struct and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_struct(const char                      *name,
		    gras_datadesc_type_cb_void_t     pre,
		    gras_datadesc_type_cb_void_t     post,
		    gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  long int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch]			= 0;
    res->alignment[arch]		= 0;
    res->aligned_size[arch]		= 0;
  }
  res->category_code		= e_gras_datadesc_type_cat_struct;
  TRY(gras_dynar_new(&(res->category.struct_data.fields),
		     sizeof(gras_dd_cat_field_t*),
		     &gras_dd_cat_field_free));
  res->pre			= pre;
  res->post			= post;

  return no_error;
}

/**
 * gras_ddt_new_struct_append:
 *
 * Append a field to the struct
 */
gras_error_t 
gras_ddt_new_struct_append(gras_datadesc_type_t            *struct_type,
			   const char                      *name,
			   gras_datadesc_type_t            *field_type,
			   gras_datadesc_type_cb_void_t     pre,
			   gras_datadesc_type_cb_void_t     post) {

  gras_error_t errcode;
  gras_dd_cat_field_t *field;
  int arch;

  gras_assert0(field_type->size >= 0,
	       "Cannot add a dynamically sized field in a structure");
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);

  for (arch=0; arch<gras_arch_count; arch ++) {
    field->offset[arch] = aligned(struct_type->size[arch], field_type->alignment[arch]);
  }
  field->code   = field_type->code;
  field->pre    = pre;
  field->post   = post;
  
  TRY(gras_dynar_push(struct_type->category.struct_data.fields, &field));

  for (arch=0; arch<gras_arch_count; arch ++) {
    struct_type->size[arch] 		= field->offset[arch] + field_type->size[arch];
    struct_type->alignment[arch]	= max(struct_type->alignment[arch], field_type->alignment[arch]);
    struct_type->aligned_size[arch]	= aligned(struct_type->size[arch], struct_type->alignment[arch]);
  }

  return no_error;
}

/**
 * gras_ddt_new_union:
 *
 * Create a new union and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_union(const char                      *name,
		   gras_datadesc_type_cb_int_t      selector,
		   gras_datadesc_type_cb_void_t     post,
		   gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  int arch;

  gras_assert0(selector,
	       "Attempt to creat an union without field_count function");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch]			= 0;
    res->alignment[arch]		= 0;
    res->aligned_size[arch]		= 0;
  }
  res->category_code		= e_gras_datadesc_type_cat_union;
  TRY(gras_dynar_new(&(res->category.union_data.fields),
		     sizeof(gras_dd_cat_field_t*),
		     &gras_dd_cat_field_free));
  res->category.union_data.selector = selector;
  res->pre			= NULL;
  res->post			= post;

  return no_error;
}

/**
 * gras_ddt_new_union_append:
 *
 * Append a field to the union
 */
gras_error_t 
gras_ddt_new_union_append(gras_datadesc_type_t            *union_type,
			  const char                      *name,
			  gras_datadesc_type_t            *field_type,
			  gras_datadesc_type_cb_void_t     pre,
			  gras_datadesc_type_cb_void_t     post) {

  gras_error_t errcode;
  gras_dd_cat_field_t *field;
  int arch;

  gras_assert0(field_type->size >= 0,
	       "Cannot add a dynamically sized field in an union");
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);
  for (arch=0; arch<gras_arch_count; arch ++) {
    field->offset[arch] = 0; /* that's the purpose of union ;) */
  }
  field->code   = field_type->code;
  field->pre    = pre;
  field->post   = post;
  
  TRY(gras_dynar_push(union_type->category.union_data.fields, &field));

  for (arch=0; arch<gras_arch_count; arch ++) {
    union_type->size[arch] 	   = max(union_type->size[arch], field_type->size[arch]);
    union_type->alignment[arch]	   = max(union_type->alignment[arch], field_type->alignment[arch]);
    union_type->aligned_size[arch] = aligned(union_type->size[arch], union_type->alignment[arch]);
  }

  return no_error;
}

/**
 * gras_ddt_new_ref:
 *
 * Create a new ref and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_ref(const char                      *name,
		 gras_datadesc_type_t            *referenced_type,
		 gras_datadesc_type_cb_int_t      selector,
		 gras_datadesc_type_cb_void_t     post,
		 gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  gras_datadesc_type_t *pointer_type;
  int arch;

  gras_assert0(selector || referenced_type,
	       "Attempt to create a generic reference without selector");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  TRY(gras_datadesc_by_name("data pointer", &pointer_type));
      
  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch]			= pointer_type->size[arch];
    res->alignment[arch]		= pointer_type->alignment[arch];
    res->aligned_size[arch]		= pointer_type->aligned_size[arch];
  }

  res->category_code		= e_gras_datadesc_type_cat_ref;

  res->category.ref_data.code         = referenced_type ? referenced_type->code : -1;
  res->category.ref_data.selector = selector;
  res->pre			= NULL;
  res->post			= post;

  return no_error;
}

/**
 * gras_ddt_new_array:
 *
 * Create a new array and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_array(const char                      *name,
		   gras_datadesc_type_t            *element_type,
		   long int                         fixed_size,
		   gras_datadesc_type_cb_int_t      dynamic_size,
		   gras_datadesc_type_cb_void_t     post,
		   gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  int arch;

  gras_assert0(dynamic_size || fixed_size>0,
	       "Attempt to create a dynamic array without size discriminant");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
    if (fixed_size <= 0) {
      res->size[arch] = fixed_size; /* make sure it indicates "dynamic" */
    } else {
      res->size[arch] = fixed_size * element_type->aligned_size[arch];
    }
    res->alignment[arch]	= element_type->alignment[arch];
    res->aligned_size[arch]	= fixed_size; /*FIXME: That was so in GS, but looks stupid*/
  }
  res->category_code		= e_gras_datadesc_type_cat_array;

  res->category.array_data.code         = element_type->code;
  res->category.array_data.fixed_size   = fixed_size;
  res->category.array_data.dynamic_size = dynamic_size;

  res->pre			= NULL;
  res->post			= post;

  return no_error;
}

/**
 * gras_ddt_new_ignored:
 *
 * Create a new ignored field and give a pointer to it. 
 *
 * If you give a default value, it will be copied away so that you can free your copy.
 */
gras_error_t 
gras_ddt_new_ignored(const char *name,
		     void *default_value,
		     void_f_pvoid_t   *free_func,
		     long int                       size,
		     long int                       alignment,
		     gras_datadesc_type_cb_void_t     post,
		     gras_datadesc_type_t           **dst) {
  RAISE_UNIMPLEMENTED;
  /*
  gras_error_t errcode;
  gras_datadesc_type_t *res;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  res->size		= size > 0?size:0;
  res->alignment	= alignment;

  if (size > 0) {
    res->aligned_size	= aligned(size, alignment);
  } else {
    res->aligned_size	= 0;
  }

  if (default_value && res->size) {
    res->category.ignored_data.default_value = malloc((size_t)size);
    if (! (res->category.ignored_data.default_value) ) 
      RAISE_MALLOC;
    memcpy(res->category.ignored_data.default_value,
	   default_value, (size_t)size);
  }

  res->category_code = e_gras_datadesc_type_cat_ignored;
  res->category.ignored_data.free_func = free_func;

  res->post = post;


  res->size = size;

  return no_error;
  */
}

/**
 * gras_ddt_new_parse:
 *
 * Create a datadescription from the result of parsing the C type description
 */
gras_error_t
gras_ddt_new_parse(const char            *name,
		   const char            *C_statement,
		   gras_datadesc_type_t **dst) {
  RAISE_UNIMPLEMENTED;
}


gras_error_t
gras_ddt_new_from_nws(const char           *name,
		      const DataDescriptor *desc,
		      size_t                howmany,
		      gras_datadesc_type_t **dst) {
  RAISE_UNIMPLEMENTED;
}

/**
 * gras_ddt_free:
 *
 * Frees a datadescription.
 */
void gras_ddt_free(gras_datadesc_type_t **type) {
  gras_datadesc_type_t *t;

  if (type && *type) {
    t=*type;

    free(t->name);
    switch (t->category_code) {
    case e_gras_datadesc_type_cat_scalar:
    case e_gras_datadesc_type_cat_ref:
    case e_gras_datadesc_type_cat_array:
      /* nothing to free in there */
      break;

    case e_gras_datadesc_type_cat_ignored:
      if (t->category.ignored_data.free_func) {
	t->category.ignored_data.free_func(t->category.ignored_data.default_value);
      }
      break;

    case e_gras_datadesc_type_cat_struct:
      gras_dynar_free(t->category.struct_data.fields);
      break;

    case e_gras_datadesc_type_cat_union:
      gras_dynar_free(t->category.union_data.fields);
      break;
      
    default:
      /* datadesc was invalid. Killing it is like euthanasy, I guess */
      break;
    }
  }
}
