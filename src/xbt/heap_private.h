/* Copyright (c) 2004, 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_PRIVATE_H
#define _XBT_HEAP_PRIVATE_H

#include "xbt/dynar.h"          /* void_f_pvoid_t */
#include "xbt/heap.h"

typedef struct xbt_heapItem {
  void *content;
  double key;
} s_xbt_heapItem_t, *xbt_heapItem_t;

typedef struct xbt_heap {
  int size;
  int count;
  xbt_heapItem_t items;
  void_f_pvoid_t free;
  void (*update_callback) (void *, int);
} s_xbt_heap_t;

#define PARENT(i)  i/2
#define LEFT(i)    2*i
#define RIGHT(i)   2*i+1

#define KEY(H,i)     ((H->items)[i]).key
#define CONTENT(H,i) ((H->items)[i]).content

#define MIN_KEY_VALUE -10000

static void xbt_heap_maxHeapify(xbt_heap_t H);
static void xbt_heap_increaseKey(xbt_heap_t H, int i);

#endif                          /* _XBT_HEAP_PRIVATE_H */
