/* $Id$ */

/* gras/datadesc.h - Describing the data you want to exchange               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_H
#define GRAS_DATADESC_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>


/*! C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

/**
 * Basic types we can embeed in DataDescriptors.
 */
typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9

/*!  \brief Describe a collection of data.
 * 
** A description of a collection of #type# data.  #repetitions# is used only
** for arrays; it contains the number of elements.  #offset# is used only for
** struct members in host format; it contains the offset of the member from the
** beginning of the struct, taking into account internal padding added by the
** compiler for alignment purposes.  #members#, #length#, and #tailPadding# are
** used only for STRUCT_TYPE data; the #length#-long array #members# describes
** the members of the nested struct, and #tailPadding# indicates how many
** padding bytes the compiler adds to the end of the structure.
*/

typedef struct DataDescriptorStruct {
  DataTypes type;
  size_t repetitions;
  size_t offset;
  /*@null@*/ struct DataDescriptorStruct *members;
  size_t length;
  size_t tailPadding;
} DataDescriptor;
/** DataDescriptor for an array */
#define SIMPLE_DATA(type,repetitions) \
  {type, repetitions, 0, NULL, 0, 0}
/** DataDescriptor for an structure member */
#define SIMPLE_MEMBER(type,repetitions,offset) \
  {type, repetitions, offset, NULL, 0, 0}
/** DataDescriptor for padding bytes */
#define PAD_BYTES(structType,lastMember,memberType,repetitions) \
  sizeof(structType) - offsetof(structType, lastMember) - \
  sizeof(memberType) * repetitions

/*
gras_error_t gras_datadesc_parse(const char       *def,
				 gras_datadesc__t **dst);
gras_error_t gras_datadesc_from_nws(const DataDescriptor *desc,
				    size_t                howmany,
				    gras_datadesc_t     **dst);
gras_error_t gras_datadesc_sizeof_host(gras_datadesc_t *desc,
				       size_t          *dst);
gras_error_t gras_datadesc_sizeof_network(gras_datadesc_t *desc,
					  size_t          *dst);
*/

typedef struct s_gras_datadesc_type gras_datadesc_type_t;

typedef void (*gras_datadesc_type_cb_void_t)(void                 *vars,
					     gras_datadesc_type_t *p_type,
					     void                 *data);
typedef int (*gras_datadesc_type_cb_int_t)(void                 *vars,
					   gras_datadesc_type_t *p_type,
					   void                 *data);



/* Create a new type and register it on the local machine */
#define gras_datadesc_declare_struct(   name,             code) \
        gras_datadesc_declare_struct_cb(name, NULL, NULL, code)

#define gras_datadesc_declare_struct_add_name(   struct_code,field_name,field_type_name) \
        gras_datadesc_declare_struct_add_name_cb(struct_code,field_name,field_type_name, NULL, NULL)

#define gras_datadesc_declare_struct_add_code(   struct_code,field_name,field_type_code) \
        gras_datadesc_declare_struct_add_code_cb(struct_code,field_name,field_type_code, NULL, NULL)

gras_error_t 
gras_datadesc_declare_struct_cb(const char                   *name,
				gras_datadesc_type_cb_void_t  pre_cb,
				gras_datadesc_type_cb_void_t  post_cb,
				long int                     *code);
gras_error_t 
gras_datadesc_declare_struct_add_name_cb(long int                      struct_code,
					 const char                   *field_name,
					 const char                   *field_type_name,
					 gras_datadesc_type_cb_void_t  pre_cb,
					 gras_datadesc_type_cb_void_t  post_cb);

gras_error_t 
gras_datadesc_declare_struct_add_code_cb(long int                      struct_code,
					 const char                   *field_name,
					 long int                      field_code,
					 gras_datadesc_type_cb_void_t  pre_cb,
					 gras_datadesc_type_cb_void_t  post_cb);
/* union */
#define gras_datadesc_declare_union(   name,             code) \
        gras_datadesc_declare_union_cb(name, NULL, NULL, code)

#define gras_datadesc_declare_union_add_name(   union_code,field_name,field_type_name) \
        gras_datadesc_declare_union_add_name_cb(union_code,field_name,field_type_name, NULL, NULL)

#define gras_datadesc_declare_union_add_code(   union_code,field_name,field_type_code) \
        gras_datadesc_declare_union_add_code_cb(union_code,field_name,field_type_code, NULL, NULL)

gras_error_t 
gras_datadesc_declare_union_cb(const char                   *name,
			       gras_datadesc_type_cb_int_t   field_count,
			       gras_datadesc_type_cb_void_t  post,
			       long int                     *code);
gras_error_t 
gras_datadesc_declare_union_add_name_cb(long int                      union_code,
					const char                   *field_name,
					const char                   *field_type_name,
					gras_datadesc_type_cb_void_t  pre_cb,
					gras_datadesc_type_cb_void_t  post_cb);
gras_error_t 
gras_datadesc_declare_union_add_code_cb(long int                      union_code,
					const char                   *field_name,
					long int                      field_code,
					gras_datadesc_type_cb_void_t  pre_cb,
					gras_datadesc_type_cb_void_t  post_cb);
/* ref */
#define gras_datadesc_declare_ref(name,ref_type, code) \
        gras_datadesc_declare_ref_cb(name, ref_type, NULL, NULL, code)
#define gras_datadesc_declare_ref_disc(name,discriminant, code) \
        gras_datadesc_declare_ref_cb(name, NULL, discriminant,  NULL, code)

gras_error_t
gras_datadesc_declare_ref_cb(const char                      *name,
			     gras_datadesc_type_t            *referenced_type,
			     gras_datadesc_type_cb_int_t      discriminant,
			     gras_datadesc_type_cb_void_t     post,
			     long int                        *code);
/* array */
#define gras_datadesc_declare_array(name,elm_type, size, code) \
        gras_datadesc_declare_array_cb(name, elm_type, size, NULL,         NULL, code)
#define gras_datadesc_declare_array_dyn(name,elm_type, dynamic_size, code) \
        gras_datadesc_declare_array_cb(name, elm_type, -1,   dynamic_size, NULL, code)

gras_error_t 
gras_datadesc_declare_array_cb(const char                      *name,
			       gras_datadesc_type_t            *element_type,
			       long int                         fixed_size,
			       gras_datadesc_type_cb_int_t      dynamic_size,
			       gras_datadesc_type_cb_void_t     post,
			       long int                        *code);


/* Use the datadescriptions */
int
gras_datadesc_type_cmp(const gras_datadesc_type_t *d1,
		       const gras_datadesc_type_t *d2);


gras_error_t 
gras_datadesc_cpy(gras_datadesc_type_t *type, void *src, void **dst);


END_DECL

#endif /* GRAS_DATADESC_H */
