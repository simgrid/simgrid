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
  
  *dst=res;
  return no_error;
}

/**
 * gras_ddt_new_scalar:
 *
 * Create a new scalar and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_scalar(const char                       *name,
		    long int                         size,
		    enum e_gras_dd_scalar_encoding   encoding,
		    gras_datadesc_type_cb_void_t     cb,
		    gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  res->size = size>0 ? size : 0;
  
  if (size>0) { /* Black magic from Oli FIXME: documentation ;)*/
    long int sz   = size;
    long int mask = sz;
 
    while ((sz >>= 1)) {
      mask |= sz;
    }

    if (size & (mask >> 1)) {
      res->alignment = (size & ~(mask >> 1)) << 1;
      gras_assert0(res->alignment != 0,
		   "scalar type too large");
      
      res->aligned_size    = aligned(size, res->alignment);
      gras_assert0 (res->aligned_size >= 0,
		    "scalar type too large");
      
    } else {
      res->alignment       = size & ~(mask >> 1);;
      res->aligned_size    = aligned(size, res->alignment);
    }
    
  } else {
    res->alignment    = 0;
    res->aligned_size = 0;
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

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  res->size			= 0;
  res->alignment		= 0;
  res->aligned_size		= 0;
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

  gras_assert0(field_type->size >= 0,
	       "Cannot add a dynamically sized field in a structure");
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);
  field->offset = aligned(struct_type->size, field_type->alignment);
  field->code   = field_type->code;
  field->pre    = pre;
  field->post   = post;
  
  TRY(gras_dynar_push(struct_type->category.struct_data.fields, field));

  struct_type->size 		= field->offset + field_type->size;
  struct_type->alignment	= max(struct_type->alignment, field_type->alignment);
  struct_type->aligned_size	= aligned(struct_type->size, struct_type->alignment);

  return no_error;
}

/**
 * gras_ddt_new_union:
 *
 * Create a new union and give a pointer to it 
 */
gras_error_t 
gras_ddt_new_union(const char                      *name,
		   gras_datadesc_type_cb_int_t      field_count,
		   gras_datadesc_type_cb_void_t     post,
		   gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;

  gras_assert0(field_count,
	       "Attempt to creat an union without field_count function");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  res->size			= 0;
  res->alignment		= 0;
  res->aligned_size		= 0;
  res->category_code		= e_gras_datadesc_type_cat_union;
  TRY(gras_dynar_new(&(res->category.union_data.fields),
		     sizeof(gras_dd_cat_field_t*),
		     &gras_dd_cat_field_free));
  res->category.union_data.field_count = field_count;
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

  gras_assert0(field_type->size >= 0,
	       "Cannot add a dynamically sized field in an union");
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);
  field->offset = 0; /* that's the purpose of union ;) */
  field->code   = field_type->code;
  field->pre    = pre;
  field->post   = post;
  
  TRY(gras_dynar_push(union_type->category.union_data.fields, field));

  union_type->size 	   = max(union_type->size, field_type->size);
  union_type->alignment	   = max(union_type->alignment, field_type->alignment);
  union_type->aligned_size = aligned(union_type->size, union_type->alignment);

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
		 gras_datadesc_type_cb_int_t      discriminant,
		 gras_datadesc_type_cb_void_t     post,
		 gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;

  gras_assert0(discriminant || referenced_type,
	       "Attempt to create a generic reference without discriminant");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  /* FIXME: Size from bootstraping */
  res->size			= 0;
  res->alignment		= 0;
  res->aligned_size		= 0;
  res->category_code		= e_gras_datadesc_type_cat_ref;

  res->category.ref_data.code         = referenced_type ? referenced_type->code : -1;
  res->category.ref_data.discriminant = discriminant;
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

  gras_assert0(dynamic_size || fixed_size>0,
	       "Attempt to create a dynamic array without size discriminant");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  if (fixed_size <= 0) {
    res->size = fixed_size;
  } else {
    res->size = fixed_size * element_type->aligned_size;
  }
  res->alignment		= element_type->alignment;
  res->aligned_size		= fixed_size; /*FIXME: That was so in GS, but looks stupid*/
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
