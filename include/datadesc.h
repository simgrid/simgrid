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


/* datadesc */
typedef struct s_gras_datadesc_type gras_datadesc_type_t;

/* callbacks prototypes */
typedef void (*gras_datadesc_type_cb_void_t)(void                 *vars,
					     gras_datadesc_type_t *p_type,
					     void                 *data);
typedef int (*gras_datadesc_type_cb_int_t)(void                 *vars,
					   gras_datadesc_type_t *p_type,
					   void                 *data);

/***********************************************
 **** Search and retrieve declared datatype ****
 ***********************************************/
gras_error_t gras_datadesc_by_name(const char *name,
				   gras_datadesc_type_t **type);

/******************************************
 **** Declare datadescription yourself ****
 ******************************************/

gras_error_t 
gras_datadesc_declare_struct(const char            *name,
			     gras_datadesc_type_t **dst);
gras_error_t 
gras_datadesc_declare_struct_append(gras_datadesc_type_t          *struct_type,
				    const char                    *name,
				    gras_datadesc_type_t          *field_type);
gras_error_t 
gras_datadesc_declare_struct_append_name(gras_datadesc_type_t *struct_type,
					 const char           *name,
					 const char          *field_type_name);
gras_error_t 
gras_datadesc_declare_union(const char                      *name,
			    gras_datadesc_type_cb_int_t      selector,
			    gras_datadesc_type_t           **dst);
gras_error_t 
gras_datadesc_declare_union_append(gras_datadesc_type_t          *union_type,
				   const char                    *name,
				   gras_datadesc_type_t          *field_type);
gras_error_t 
gras_datadesc_declare_union_append_name(gras_datadesc_type_t *union_type,
					const char           *name,
					const char           *field_type_name);
gras_error_t 
gras_datadesc_declare_ref(const char                      *name,
			  gras_datadesc_type_t            *referenced_type,
			  gras_datadesc_type_t           **dst);
gras_error_t
gras_datadesc_declare_ref_generic(const char                   *name,
				  gras_datadesc_type_cb_int_t   discriminant,
				  gras_datadesc_type_t        **dst);
gras_error_t 
gras_datadesc_declare_array_fixed(const char                    *name,
				  gras_datadesc_type_t          *element_type,
				  long int                       fixed_size,
				  gras_datadesc_type_t         **dst);
gras_error_t 
gras_datadesc_declare_array_dyn(const char                      *name,
				gras_datadesc_type_t            *element_type,
				gras_datadesc_type_cb_int_t      dynamic_size,
				gras_datadesc_type_t           **dst);

/*********************************
 * Change stuff within datadescs *
 *********************************/

typedef struct s_gras_dd_cbps gras_dd_cbps_t;
void gras_datadesc_cb_set_pre (gras_datadesc_type_t    *type,
			       gras_datadesc_type_cb_void_t  pre);
void gras_datadesc_cb_set_post(gras_datadesc_type_t    *type,
			       gras_datadesc_type_cb_void_t  post);

/********************************************************
 * Advanced data describing: callback persistent states *
 ********************************************************/

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
		    gras_datadesc_type_t **dst);
#define GRAS_DEFINE_TYPE(name,def) \
  static const char * _gras_this_type_symbol_does_not_exist__##name=#def; def
 
/*#define gras_type_symbol_parse(name)                                  \
 _gras_datadesc_parse(_gras_this_datadesc_type_symbol_does_not_exist__##name)
*/ 
#define gras_datadesc_by_symbol(name)                    \
  (bag->bag_ops->get_type_by_name(bag, NULL, #name) ?    \
     bag->bag_ops->get_type_by_name(bag, NULL, #name) :  \
     gras_datadesc_parse(name)                           \
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
gras_datadesc_import_nws(const char           *name,
			 const DataDescriptor *desc,
			 size_t                howmany,
			 gras_datadesc_type_t **dst);

END_DECL

#endif /* GRAS_DATADESC_H */
