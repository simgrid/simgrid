/* $Id$ */

/* ddt_new - creation/deletion of datatypes structs (private to this module)*/

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(create,datadesc);

/**
 * gras_ddt_freev:
 *
 * gime that memory back, dude. I mean it.
 */
void gras_ddt_freev(void *ddt) {
  gras_datadesc_type_t *type= (gras_datadesc_type_t *)ddt;
  
  if (type) {
    gras_datadesc_unref(type);
  }
}

static gras_error_t
gras_ddt_new(const char            *name,
	     gras_datadesc_type_t **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;

  res=malloc(sizeof(gras_datadesc_type_t));
  if (!res) 
    RAISE_MALLOC;

  memset(res, 0, sizeof(gras_datadesc_type_t));
  res->name = strdup(name);
  res->name_len = strlen(name);
  res->refcounter = 1;
  
  TRY(gras_set_add(gras_datadesc_set_local,
		   (gras_set_elm_t*)res,&gras_ddt_freev));
    
  *dst=res;
  return no_error;
}

/**
 * gras_datadesc_by_name:
 *
 * Retrieve a type from its name
 */
gras_datadesc_type_t *gras_datadesc_by_name(const char *name) {

  gras_datadesc_type_t *type;

  if (gras_set_get_by_name(gras_datadesc_set_local,
			   name,(gras_set_elm_t**)&type) == no_error) {
    return type;
  } else { 
    return NULL;
  }
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
 * gras_datadesc_declare_scalar:
 *
 * Create a new scalar and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_scalar(const char                      *name,
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
    res->size[arch] = gras_arches[arch].sizeof_scalars[type];

    sz = res->size[arch];
    mask = sz;
    
    /* just in case you wonder, x>>1 == x/2 on all architectures when x>=0
       and a size is always>=0 */
    
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
      
      res->aligned_size[arch] = aligned(res->size[arch], res->alignment[arch]);
      gras_assert0 (res->aligned_size[arch] >= 0,
		    "scalar type too large");
      
    } else {
      /* size is a power of 2, life is great */
      res->alignment[arch]       = res->size[arch];
      res->aligned_size[arch]    = res->size[arch];
    }
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
  gras_dd_cat_field_t *field = *(gras_dd_cat_field_t **)f;
  if (field) {
    if (field->name) 
      free(field->name);
    free(field);
  }
}

/**
 * gras_datadesc_declare_struct:
 *
 * Create a new struct and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_struct(const char                      *name,
			gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  long int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = 0;
    res->alignment[arch] = 0;
    res->aligned_size[arch] = 0;
  }
  res->category_code = e_gras_datadesc_type_cat_struct;
  TRY(gras_dynar_new(&(res->category.struct_data.fields),
		     sizeof(gras_dd_cat_field_t*),
		     &gras_dd_cat_field_free));

  return no_error;
}

/**
 * gras_datadesc_declare_struct_append:
 *
 * Append a field to the struct
 */
gras_error_t 
gras_datadesc_declare_struct_append(gras_datadesc_type_t  *struct_type,
				    const char            *name,
				    gras_datadesc_type_t  *field_type) {

  gras_error_t errcode;
  gras_dd_cat_field_t *field;
  int arch;
 
  gras_assert1(!struct_type->category.struct_data.closed,
	       "Cannot add anything to the already closed struct %s",
	       struct_type->name);
  gras_assert1(field_type->size >= 0,
	       "Cannot add a dynamically sized field in structure %s",
	       struct_type->name);
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);

  DEBUG0("----------------");
  DEBUG4("PRE s={size=%ld,align=%ld,asize=%ld} struct_boundary=%d",
	 struct_type->size[GRAS_THISARCH], 
	 struct_type->alignment[GRAS_THISARCH], 
	 struct_type->aligned_size[GRAS_THISARCH],
	 gras_arches[GRAS_THISARCH].struct_boundary);
     
     
  for (arch=0; arch<gras_arch_count; arch ++) {
    field->offset[arch] = aligned(struct_type->size[arch],
				  min(field_type->alignment[arch],
				      gras_arches[arch].struct_boundary));

    struct_type->size[arch] = field->offset[arch] + field_type->size[arch];
    struct_type->alignment[arch] = max(struct_type->alignment[arch],
				       field_type->alignment[arch]);
    struct_type->aligned_size[arch] = aligned(struct_type->size[arch],
					      struct_type->alignment[arch]);
  }
  field->code   = field_type->code;
  field->pre    = NULL;
  field->post   = NULL;
  
  TRY(gras_dynar_push(struct_type->category.struct_data.fields, &field));

  DEBUG3("Push a %s into %s at offset %ld.",
	 field_type->name, struct_type->name,field->offset[GRAS_THISARCH]);
  DEBUG3("  f={size=%ld,align=%ld,asize=%ld}",
	 field_type->size[GRAS_THISARCH], 
	 field_type->alignment[GRAS_THISARCH], 
	 field_type->aligned_size[GRAS_THISARCH]);
  DEBUG3("  s={size=%ld,align=%ld,asize=%ld}",
	 struct_type->size[GRAS_THISARCH], 
	 struct_type->alignment[GRAS_THISARCH], 
	 struct_type->aligned_size[GRAS_THISARCH]);
  return no_error;
}
void
gras_datadesc_declare_struct_close(gras_datadesc_type_t  *struct_type) {
   struct_type->category.struct_data.closed = 1;
   //   INFO0("FIXME: Do something in gras_datadesc_declare_struct_close");
}

/**
 * gras_datadesc_declare_union:
 *
 * Create a new union and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_union(const char                   *name,
			    gras_datadesc_type_cb_int_t   selector,
			    gras_datadesc_type_t        **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  int arch;

  gras_assert0(selector,
	       "Attempt to creat an union without field_count function");

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
     res->size[arch] = 0;
     res->alignment[arch] = 0;
     res->aligned_size[arch] = 0;
  }

  res->category_code		= e_gras_datadesc_type_cat_union;
  TRY(gras_dynar_new(&(res->category.union_data.fields),
		     sizeof(gras_dd_cat_field_t*),
		     &gras_dd_cat_field_free));
  res->category.union_data.selector = selector;

  return no_error;
}

/**
 * gras_datadesc_declare_union_append:
 *
 * Append a field to the union
 */
gras_error_t 
gras_datadesc_declare_union_append(gras_datadesc_type_t  *union_type,
				   const char            *name,
				   gras_datadesc_type_t  *field_type) {

  gras_error_t errcode;
  gras_dd_cat_field_t *field;
  int arch;

  gras_assert1(!union_type->category.union_data.closed,
	       "Cannot add anything to the already closed union %s",
	       union_type->name);
  gras_assert1(field_type->size >= 0,
	       "Cannot add a dynamically sized field in union %s",
	       union_type->name);
    
  field=malloc(sizeof(gras_dd_cat_field_t));
  if (!field)
    RAISE_MALLOC;

  field->name   = strdup(name);
  for (arch=0; arch<gras_arch_count; arch ++) {
    field->offset[arch] = 0; /* that's the purpose of union ;) */
  }
  field->code   = field_type->code;
  field->pre    = NULL;
  field->post   = NULL;
  
  TRY(gras_dynar_push(union_type->category.union_data.fields, &field));

  for (arch=0; arch<gras_arch_count; arch ++) {
    union_type->size[arch] = max(union_type->size[arch],
				 field_type->size[arch]);
    union_type->alignment[arch] = max(union_type->alignment[arch],
				      field_type->alignment[arch]);
    union_type->aligned_size[arch] = aligned(union_type->size[arch],
					     union_type->alignment[arch]);
  }
  return no_error;
}

void
gras_datadesc_declare_union_close(gras_datadesc_type_t  *union_type) {
   union_type->category.union_data.closed = 1;
   //   INFO0("FIXME: Do something in gras_datadesc_declare_union_close");
}
/**
 * gras_datadesc_declare_ref:
 *
 * Create a new ref to a fixed type and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_ref(const char             *name,
			  gras_datadesc_type_t   *referenced_type,
			  gras_datadesc_type_t  **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  gras_datadesc_type_t *pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  gras_assert0(pointer_type, "Cannot get the description of data pointer");
      
  for (arch=0; arch<gras_arch_count; arch ++){
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }

  res->category_code		= e_gras_datadesc_type_cat_ref;

  res->category.ref_data.code     = referenced_type->code;
  res->category.ref_data.selector = NULL;

  return no_error;
}
/**
 * gras_datadesc_declare_ref_generic:
 *
 * Create a new ref to a type given at use time, and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_ref_generic(const char                      *name,
			 gras_datadesc_type_cb_int_t      selector,
			 gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  gras_datadesc_type_t *pointer_type = gras_datadesc_by_name("data pointer");
  int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  gras_assert0(pointer_type, "Cannot get the description of data pointer");
      
  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = pointer_type->size[arch];
    res->alignment[arch] = pointer_type->alignment[arch];
    res->aligned_size[arch] = pointer_type->aligned_size[arch];
  }

  res->category_code		= e_gras_datadesc_type_cat_ref;

  res->category.ref_data.code     = -1;
  res->category.ref_data.selector = selector;

  return no_error;
}

/**
 * gras_datadesc_declare_array_fixed:
 *
 * Create a new array and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_array_fixed(const char              *name,
				  gras_datadesc_type_t    *element_type,
				  long int                 fixed_size,
				  gras_datadesc_type_t   **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  int arch;

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  gras_assert1(fixed_size > 0, "'%s' is a array of negative fixed size",name);
  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = fixed_size * element_type->aligned_size[arch];
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = res->size[arch];
  }  

  res->category_code		= e_gras_datadesc_type_cat_array;

  res->category.array_data.code         = element_type->code;
  res->category.array_data.fixed_size   = fixed_size;
  res->category.array_data.dynamic_size = NULL;

  return no_error;
}
/**
 * gras_datadesc_declare_array_dyn:
 *
 * Create a new array and give a pointer to it 
 */
gras_error_t 
gras_datadesc_declare_array_dyn(const char                      *name,
		       gras_datadesc_type_t            *element_type,
		       gras_datadesc_type_cb_int_t      dynamic_size,
		       gras_datadesc_type_t           **dst) {

  gras_error_t errcode;
  gras_datadesc_type_t *res;
  int arch;

  gras_assert1(dynamic_size,
	       "'%s' is a dynamic array without size discriminant",
	       name);

  TRY(gras_ddt_new(name,dst));
  res=*dst;

  for (arch=0; arch<gras_arch_count; arch ++) {
    res->size[arch] = -1; /* make sure it indicates "dynamic" */
    res->alignment[arch] = element_type->alignment[arch];
    res->aligned_size[arch] = -1; /*FIXME: That was so in GS, but looks stupid*/
  }

  res->category_code		= e_gras_datadesc_type_cat_array;

  res->category.array_data.code         = element_type->code;
  res->category.array_data.fixed_size   = -1;
  res->category.array_data.dynamic_size = dynamic_size;

  return no_error;
}

gras_error_t
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 size_t                howmany,
			 gras_datadesc_type_t **dst) {
  RAISE_UNIMPLEMENTED;
}

/**
 * gras_datadesc_cb_set_pre:
 *
 * Add a pre-send callback to this datadexc
 */
void gras_datadesc_cb_set_pre (gras_datadesc_type_t         *type,
			       gras_datadesc_type_cb_void_t  pre) {
  type->pre = pre;
}
/**
 * gras_datadesc_cb_set_post:
 *
 * Add a post-send callback to this datadexc
 */
void gras_datadesc_cb_set_post(gras_datadesc_type_t         *type,
			       gras_datadesc_type_cb_void_t  post) {
  type->post = post;
}

/**
 * gras_datadesc_ref:
 *
 * Adds a reference to the datastruct. 
 * ddt will be freed only when the refcount becomes 0.
 */
void gras_datadesc_ref(gras_datadesc_type_t *type) {
  type->refcounter ++;
}

/**
 * gras_datadesc_unref:
 *
 * Adds a reference to the datastruct. 
 * ddt will be freed only when the refcount becomes 0.
 */
void gras_datadesc_unref(gras_datadesc_type_t *type) {
  type->refcounter--;
  if (!type->refcounter) {
    /* even the set of ddt released that type. Let's free it */
    DEBUG1("Let's free ddt %s",type->name);

    free(type->name);
    switch (type->category_code) {
    case e_gras_datadesc_type_cat_scalar:
    case e_gras_datadesc_type_cat_ref:
    case e_gras_datadesc_type_cat_array:
      /* nothing to free in there */
      break;

    case e_gras_datadesc_type_cat_ignored:
      if (type->category.ignored_data.free_func) {
	type->category.ignored_data.free_func
	  (type->category.ignored_data.default_value);
      }
      break;

    case e_gras_datadesc_type_cat_struct:
      gras_dynar_free(type->category.struct_data.fields);
      break;

    case e_gras_datadesc_type_cat_union:
      gras_dynar_free(type->category.union_data.fields);
      break;
      
    default:
      /* datadesc was invalid. Killing it is like euthanasy, I guess */
      break;
    }
    free(type);
  }
}
