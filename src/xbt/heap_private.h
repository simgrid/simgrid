/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_PRIVATE_H
#define _XBT_HEAP_PRIVATE_H

#include "xbt/dynar.h"          /* void_f_pvoid_t */
#include "xbt/heap.h"

typedef struct xbt_heap_item {
  void *content;
  double key;
} s_xbt_heap_item_t, *xbt_heap_item_t;

typedef struct xbt_heap {
  int size;
  int count;
  s_xbt_heap_item_t* items; /* array of structs */
  void_f_pvoid_t free;
  void (*update_callback) (void *, int);
} s_xbt_heap_t;

#define PARENT(i)  (i >> 1)
#define LEFT(i)    (i << 1)
#define RIGHT(i)   ((i << 1) + 1)

#define KEY(H,i)     ((H->items)[i]).key
#define CONTENT(H,i) ((H->items)[i]).content

#define MIN_KEY_VALUE -10000

#endif                          /* _XBT_HEAP_PRIVATE_H */
