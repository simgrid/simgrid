/* $Id$ */

/* ddt_declare - user functions to create datatypes on locale machine       */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 the GRAS posse.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

gras_error_t 
gras_datadesc_declare_struct_cb(const char                   *name,
				gras_datadesc_type_cb_void_t  pre,
				gras_datadesc_type_cb_void_t  post,
				long int                     *code) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_struct(name, pre, post, &type));
  TRY(gras_ddt_register(type));
  *code = type->code;
  return no_error;
}

gras_error_t 
gras_datadesc_declare_struct_add_name_cb(long int    struct_code,
					 const char *field_name,
					 const char *field_type_name,
					 gras_datadesc_type_cb_void_t     pre_cb,
					 gras_datadesc_type_cb_void_t     post_cb) {

  gras_error_t errcode;
  gras_datadesc_type_t *struct_type;
  gras_datadesc_type_t *field_type;
  
  TRY(gras_set_get_by_id(gras_datadesc_set_local,struct_code,
			 (gras_set_elm_t**)struct_type));
  TRY(gras_set_get_by_name(gras_datadesc_set_local,field_type_name,
			 (gras_set_elm_t**)field_type));

  TRY(gras_ddt_new_struct_append(struct_type,
				 field_name,  field_type,
				 pre_cb,      post_cb));
  
  return no_error;
}
gras_error_t 
gras_datadesc_declare_struct_add_code_cb(long int    struct_code,
					 const char *field_name,
					 long int    field_code,
					 gras_datadesc_type_cb_void_t     pre_cb,
					 gras_datadesc_type_cb_void_t     post_cb) {
  gras_error_t errcode;
  gras_datadesc_type_t *struct_type;
  gras_datadesc_type_t *field_type;
  
  TRY(gras_set_get_by_id(gras_datadesc_set_local,struct_code,
			 (gras_set_elm_t**)struct_type));
  TRY(gras_set_get_by_id(gras_datadesc_set_local,field_code,
			 (gras_set_elm_t**)field_type));

  TRY(gras_ddt_new_struct_append(struct_type,
				 field_name,  field_type,
				 pre_cb,      post_cb));
  
  return no_error;
}

gras_error_t 
gras_datadesc_declare_union_cb(const char                   *name,
			       gras_datadesc_type_cb_int_t   field_count,
			       gras_datadesc_type_cb_void_t  post,
			       long int                     *code) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_union(name, field_count, post, &type));
  TRY(gras_ddt_register(type));
  *code = type->code;
  return no_error;
}

gras_error_t 
gras_datadesc_declare_union_add_name_cb(long int                      union_code,
					const char                   *field_name,
					const char                   *field_type_name,
					gras_datadesc_type_cb_void_t  pre_cb,
					gras_datadesc_type_cb_void_t  post_cb) {
  gras_error_t errcode;
  gras_datadesc_type_t *union_type;
  gras_datadesc_type_t *field_type;
  
  TRY(gras_set_get_by_id(gras_datadesc_set_local,union_code,
			 (gras_set_elm_t**)union_type));
  TRY(gras_set_get_by_name(gras_datadesc_set_local,field_type_name,
			 (gras_set_elm_t**)field_type));

  TRY(gras_ddt_new_union_append(union_type,
				field_name,  field_type,
				pre_cb,      post_cb));
  
  return no_error;
}
gras_error_t 
gras_datadesc_declare_union_add_code_cb(long int                      union_code,
					const char                   *field_name,
					long int                      field_code,
					gras_datadesc_type_cb_void_t  pre_cb,
					gras_datadesc_type_cb_void_t  post_cb) {
  gras_error_t errcode;
  gras_datadesc_type_t *union_type;
  gras_datadesc_type_t *field_type;
  
  TRY(gras_set_get_by_id(gras_datadesc_set_local,union_code,
			 (gras_set_elm_t**)union_type));
  TRY(gras_set_get_by_id(gras_datadesc_set_local,field_code,
			 (gras_set_elm_t**)field_type));

  TRY(gras_ddt_new_union_append(union_type,
				field_name,  field_type,
				pre_cb,      post_cb));
  
  return no_error;
}

gras_error_t
gras_datadesc_declare_ref_cb(const char                      *name,
			     gras_datadesc_type_t            *referenced_type,
			     gras_datadesc_type_cb_int_t      discriminant,
			     gras_datadesc_type_cb_void_t     post,
			     long int                        *code){
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_ref(name, referenced_type,discriminant,post, &type));
  TRY(gras_ddt_register(type));
  *code = type->code;
  return no_error;
}

gras_error_t 
gras_datadesc_declare_array_cb(const char                      *name,
			       gras_datadesc_type_t            *element_type,
			       long int                         fixed_size,
			       gras_datadesc_type_cb_int_t      dynamic_size,
			       gras_datadesc_type_cb_void_t     post,
			       long int                        *code){
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_array(name, element_type, fixed_size, dynamic_size, post, &type));
  TRY(gras_ddt_register(type));
  *code = type->code;
  return no_error;
}

/**
 * gras_datadesc_parse:
 *
 * Parse a C type declaration, and declare locally the corresponding type description
 */
gras_error_t
gras_datadesc_parse(const char *name,
		    const char *definition,
		    long int   *code) {
  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_parse(name,definition,&type));
  TRY(gras_ddt_register( type));
  *code = type->code;
  return no_error;
}

/**
 * gras_datadesc_parse:
 *
 * Parse a NWS type declaration, and declare locally the corresponding type description
 */
gras_error_t
gras_datadesc_from_nws(const char           *name,
		       const DataDescriptor *desc,
		       size_t                howmany,
		       long int             *code) {

  gras_error_t errcode;
  gras_datadesc_type_t *type;
  TRY(gras_ddt_new_from_nws(name,desc,howmany,&type));
  TRY(gras_ddt_register(type));
  *code = type->code;
  return no_error;
}
