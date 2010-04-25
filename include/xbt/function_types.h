/* function_type.h - classical types for pointer to function                */

/* Copyright (c) 2004-2006 Martin Quinson.                                  */
/* Copyright (c) 2004 Arnaud Legrand.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTION_TYPE_H
#define XBT_FUNCTION_TYPE_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

typedef void (*void_f_ppvoid_t) (void **);
typedef void (*void_f_pvoid_t) (void *);
typedef void (*void_f_int_pvoid_t) (int,void *);
     typedef void *(*pvoid_f_void_t) (void);
     typedef void *(*pvoid_f_pvoid_t) (void *);
     typedef void (*void_f_void_t) (void);

     typedef int (*int_f_void_t) (void);

     typedef int (*int_f_pvoid_pvoid_t) (void *, void *);

     typedef int (*xbt_main_func_t) (int argc, char *argv[]);

SG_END_DECL()
#endif /* XBT_FUNCTION_TYPE_H */
