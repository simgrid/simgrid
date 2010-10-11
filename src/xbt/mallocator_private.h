/* mallocator - recycle objects to avoid malloc() / free()                  */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_MALLOCATOR_PRIVATE_H__
#define _XBT_MALLOCATOR_PRIVATE_H__

typedef struct s_xbt_mallocator {
  int current_size;             /* number of objects currently stored */
  void **objects;               /* objects stored by the mallocator and available for the user */
  int max_size;                 /* maximum number of objects */
  pvoid_f_void_t new_f;         /* function to call when we are running out of objects */
  void_f_pvoid_t free_f;        /* function to call when we have got too many objects */
  void_f_pvoid_t reset_f;       /* function to call when an object is released by the user */
} s_xbt_mallocator_t;

#endif                          /* _XBT_MALLOCATOR_PRIVATE_H__ */
