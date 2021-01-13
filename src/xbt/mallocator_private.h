/* mallocator - recycle objects to avoid malloc() / free()                  */

/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MALLOCATOR_PRIVATE_H
#define XBT_MALLOCATOR_PRIVATE_H

#include <stdatomic.h>
#include <xbt/function_types.h>

typedef struct s_xbt_mallocator {
  void **objects;               /* objects stored by the mallocator and available for the user */
  int current_size;             /* number of objects currently stored */
  int max_size;                 /* maximum number of objects */
  pvoid_f_void_t new_f;         /* function to call when we are running out of objects */
  void_f_pvoid_t free_f;        /* function to call when we have got too many objects */
  void_f_pvoid_t reset_f;       /* function to call when an object is released by the user */
  atomic_flag lock;             /* lock to ensure the mallocator is thread-safe */
} s_xbt_mallocator_t;

#endif
