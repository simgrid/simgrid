/* $Id$ */

/* ddt_use - use of datatypes structs (public)                              */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

//GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(use,DataDesc);

/**
 * gras_datadesc_get_id_from_name:
 * Returns: -1 in case of error.
 *
 * Retrieve the ID of a previously declared datatype from its name.
 */
long int
gras_datadesc_get_id_from_name(const char *name) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;

  errcode = gras_ddt_get_by_name(name,&type);
  if (errcode != no_error)
    return -1;
  return type->code;
}

/**
 * gras_datadesc_type_cmp:
 *
 * Compares two datadesc types with the same semantic than strcmp.
 *
 * This comparison does not take the set headers into account (name and ID), 
 * but only the payload (actual type description).
 */
int gras_datadesc_type_cmp(const gras_datadesc_type_t *d1,
			   const gras_datadesc_type_t *d2) {
  int ret,cpt;
  gras_dd_cat_field_t *field1,*field2;
  gras_datadesc_type_t *field_desc_1,*field_desc_2;


  if (!d1 && d2) return 1;
  if (!d1 && !d2) return 0;
  if ( d1 && !d2) return -1;

  if (d1->size          != d2->size     )     return d1->size          > d2->size         ? 1 : -1;
  if (d1->alignment     != d2->alignment)     return d1->alignment     > d2->alignment    ? 1 : -1;
  if (d1->aligned_size  != d2->aligned_size)  return d1->aligned_size  > d2->aligned_size ? 1 : -1;

  if (d1->category_code != d2->category_code) return d1->category_code > d2->category_code ? 1 : -1;

  if (d1->pre           != d2->pre)           return d1->pre           > d2->pre ? 1 : -1;
  if (d1->post          != d2->post)          return d1->post          > d2->post ? 1 : -1;

  switch (d1->category_code) {
  case e_gras_datadesc_type_cat_scalar:
    if (d1->category.scalar_data.encoding != d2->category.scalar_data.encoding)
      return d1->category.scalar_data.encoding > d2->category.scalar_data.encoding ? 1 : -1 ;
    break;
    
  case e_gras_datadesc_type_cat_struct:    
    if (gras_dynar_length(d1->category.struct_data.fields) != 
	gras_dynar_length(d2->category.struct_data.fields))
      return gras_dynar_length(d1->category.struct_data.fields) >
	gras_dynar_length(d2->category.struct_data.fields) ?
	1 : -1;
    
    gras_dynar_foreach(d1->category.struct_data.fields, cpt, field1) {
      
      gras_dynar_get(d2->category.struct_data.fields, cpt, field2);
      gras_ddt_get_by_code(field1->code,&field_desc_1); /* FIXME: errcode ignored */
      gras_ddt_get_by_code(field2->code,&field_desc_2);
      ret = gras_datadesc_type_cmp(field_desc_1,field_desc_2);
      if (ret)
	return ret;
      
    }
    break;
    
  case e_gras_datadesc_type_cat_union:
    if (d1->category.union_data.field_count != d2->category.union_data.field_count) 
      return d1->category.union_data.field_count > d2->category.union_data.field_count ? 1 : -1;
    
    if (gras_dynar_length(d1->category.union_data.fields) != 
	gras_dynar_length(d2->category.union_data.fields))
      return gras_dynar_length(d1->category.union_data.fields) >
	     gras_dynar_length(d2->category.union_data.fields) ?
	1 : -1;
    
    gras_dynar_foreach(d1->category.union_data.fields, cpt, field1) {
      
      gras_dynar_get(d2->category.union_data.fields, cpt, field2);
      gras_ddt_get_by_code(field1->code,&field_desc_1); /* FIXME: errcode ignored */
      gras_ddt_get_by_code(field2->code,&field_desc_2);
      ret = gras_datadesc_type_cmp(field_desc_1,field_desc_2);
      if (ret)
	return ret;
      
    }
    break;
    
    
  case e_gras_datadesc_type_cat_ref:
    if (d1->category.ref_data.discriminant != d2->category.ref_data.discriminant) 
      return d1->category.ref_data.discriminant > d2->category.ref_data.discriminant ? 1 : -1;
    
    if (d1->category.ref_data.code != d2->category.ref_data.code) 
      return d1->category.ref_data.code > d2->category.ref_data.code ? 1 : -1;
    break;
    
  case e_gras_datadesc_type_cat_array:
    if (d1->category.array_data.code != d2->category.array_data.code) 
      return d1->category.array_data.code > d2->category.array_data.code ? 1 : -1;
    
    if (d1->category.array_data.fixed_size != d2->category.array_data.fixed_size) 
      return d1->category.array_data.fixed_size > d2->category.array_data.fixed_size ? 1 : -1;
    
    if (d1->category.array_data.dynamic_size != d2->category.array_data.dynamic_size) 
      return d1->category.array_data.dynamic_size > d2->category.array_data.dynamic_size ? 1 : -1;
    
    break;
    
  case e_gras_datadesc_type_cat_ignored:
    /* That's ignored... */
  default:
    /* two stupidly created ddt are equally stupid ;) */
    break;
  }
  return 0;
  
}

/**
 * gras_datadesc_cpy:
 *
 * Copy the data pointed by src and described by type to a new location, and store a pointer to it in dst.
 *
 */
gras_error_t gras_datadesc_cpy(gras_datadesc_type_t *type, void *src, void **dst) {
  RAISE_UNIMPLEMENTED;
}

/**
 * gras_datadesc_send:
 *
 * Copy the data pointed by src and described by type to the socket
 *
 */
gras_error_t gras_datadesc_send(gras_socket_t *sock, gras_datadesc_type_t *type, void *src) {

  RAISE_UNIMPLEMENTED;
}

/**
 * gras_datadesc_recv:
 *
 * Get an instance of the datatype described by @type from the @socket, and store a pointer to it in @dst
 *
 */
gras_error_t
gras_datadesc_recv(gras_socket_t *sock, gras_datadesc_type_t *type, void **dst) {
  RAISE_UNIMPLEMENTED;
}
