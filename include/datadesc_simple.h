/* $Id$ */

/* gras/datadesc.h - Describing the data you want to exchange               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_DATADESC_SIMPLE_H
#define GRAS_DATADESC_SIMPLE_H

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

/****
 **** The NWS type and constructors 
 ****/

typedef enum
  {CHAR_TYPE, DOUBLE_TYPE, FLOAT_TYPE, INT_TYPE, LONG_TYPE, SHORT_TYPE,
   UNSIGNED_INT_TYPE, UNSIGNED_LONG_TYPE, UNSIGNED_SHORT_TYPE, STRUCT_TYPE}
  DataTypes;
#define SIMPLE_TYPE_COUNT 9

typedef struct DataDescriptorStruct {
  DataTypes type;
  size_t repetitions;
  size_t offset;
  struct DataDescriptorStruct *members;
  size_t length;
  size_t tailPadding;
} DataDescriptor;
#ifndef NULL
#define NULL 0
#endif
#define SIMPLE_DATA(type,repetitions) {type, repetitions, 0, NULL, 0, 0}
#define SIMPLE_MEMBER(type,repetitions,offset) \
  {type, repetitions, offset, NULL, 0, 0}
#define PAD_BYTES(structType,lastMember,memberType,repetitions) \
  sizeof(structType) - offsetof(structType, lastMember) - \
  sizeof(memberType) * repetitions

/****
 **** Gras (opaque) type, constructors and functions
 ****/

typedef struct gras_datadesc_ gras_datadesc_t;

/* constructors, memory management */
gras_error_t gras_datadesc_parse(const char       *def,
				 gras_datadesc_t **dst);
gras_error_t gras_datadesc_from_nws(const DataDescriptor *desc,
				    size_t                howmany,
				    gras_datadesc_t     **dst);

gras_error_t gras_datadesc_cpy(gras_datadesc_t  *src,
			       gras_datadesc_t **dst);
void         gras_datadesc_free(gras_datadesc_t **dd);

/* basic functionnalities */
int          gras_datadesc_cmp(const gras_datadesc_t *d1,
			       const gras_datadesc_t *d2);

gras_error_t gras_datadesc_sizeof_host(gras_datadesc_t *desc,
				       size_t          *dst);
gras_error_t gras_datadesc_sizeof_network(gras_datadesc_t *desc,
					  size_t          *dst);

/* high level function needed in SG */
gras_error_t gras_datadesc_data_cpy(const gras_datadesc_t *dd,
				    const void *src,
				    void **dst);

/* high level functions needed in RL */
gras_error_t gras_datadesc_convert_recv(const gras_datadesc_t *dd,
					gras_trp_plugin_t *trp,
					void **dst);
gras_error_t gras_datadesc_convert_send(const gras_datadesc_t *dd,
					gras_trp_plugin_t *trp,
					void *src);

END_DECL

#endif /* GRAS_DATADESC_SIMPLE_H */

