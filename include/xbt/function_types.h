/* function_type.h - classical types for pointer to function                */

/* Copyright (c) 2006-2007, 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTION_TYPE_H
#define XBT_FUNCTION_TYPE_H

#include "xbt/base.h"

SG_BEGIN_DECL()

typedef void (*void_f_ppvoid_t) (void **);
typedef void (*void_f_pvoid_t) (void *);
typedef void (*void_f_int_pvoid_t) (int, void *);
typedef void *(*pvoid_f_void_t) (void);
typedef void *(*pvoid_f_pvoid_t) (void *);
typedef void (*void_f_void_t) (void);

typedef int (*int_f_void_t) (void);
typedef int (*int_f_pvoid_t) (void*);

typedef int (*int_f_pvoid_pvoid_t) (void *, void *);
typedef int (*int_f_cpvoid_cpvoid_t) (const void *, const void *);

typedef int (*xbt_main_func_t) (int argc, char *argv[]);

typedef double(*double_f_pvoid_t) (void *);
typedef double(*double_f_cpvoid_t) (const void *);

SG_END_DECL()
#endif                          /* XBT_FUNCTION_TYPE_H */
