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


typedef struct s_gras_datadesc_type gras_datadesc_type_t;

/***********************************************
 **** Search and retrieve declared datatype ****
 ***********************************************/
long int gras_datadesc_get_id_from_name(const char *name);

gras_error_t gras_datadesc_by_name(const char *name,
				   gras_datadesc_type_t **type);
gras_error_t gras_datadesc_by_id  (long int code,
				   gras_datadesc_type_t **type);
#define gras_datadesc_by_code(a,b) gras_datadesc_by_id(a,b)

/*********************************************
 **** DataDesc callback persistent states ****
 *********************************************/
typedef struct s_gras_dd_cbps gras_dd_cbps_t;

void *
gras_dd_cbps_pop (gras_dd_cbps_t        *ps, 
		  const char            *name,
		  gras_datadesc_type_t **ddt);
void
gras_dd_cbps_push(gras_dd_cbps_t        *ps,
		  const char            *name,
		  void                  *data,
		  gras_datadesc_type_t  *ddt);
void
gras_dd_cbps_set (gras_dd_cbps_t        *ps,
		  const char            *name,
		  void                  *data,
		  gras_datadesc_type_t  *ddt);

void *
gras_dd_cbps_get (gras_dd_cbps_t        *ps, 
		  const char            *name,
		  gras_datadesc_type_t **ddt);

void
gras_dd_cbps_block_begin(gras_dd_cbps_t *ps);
void
gras_dd_cbps_block_end(gras_dd_cbps_t *ps);


/******************************************
 **** Declare datadescription manually ****
 ******************************************/

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

/*******************************
 **** About data convertion ****
 *******************************/
int gras_arch_selfid(void); /* ID of this arch */

/****************************
 **** Parse C statements ****
 ****************************/
gras_error_t
gras_datadesc_parse(const char *name,
		    const char *Cdefinition,
		    long int   *code);
#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gras_this_type_symbol_does_not_exist__##name=#def; def
 
#define gras_type_symbol_parse(bag,name)                                  \
 _gs_type_parse(bag, _gs_this_type_symbol_does_not_exist__##name)
 
#define gs_type_get_by_symbol(bag,name)                  \
  (bag->bag_ops->get_type_by_name(bag, NULL, #name) ?    \
     bag->bag_ops->get_type_by_name(bag, NULL, #name) :  \
     gras_type_symbol_parse(bag, name)                   \
  )

/*****************************
 **** NWS datadescription ****
 *****************************/

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

gras_error_t
gras_datadesc_from_nws(const char           *name,
		       const DataDescriptor *desc,
		       size_t                howmany,
		       long int             *code);


END_DECL

#endif /* GRAS_DATADESC_H */
