/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_H
#define _XBT_HEAP_H

#include "xbt/misc.h"
#include "xbt/dynar.h"          /* void_f_pvoid_t */

/** @addtogroup XBT_heap
 *  @brief This section describes the API to generic heap with O(log(n)) access.
 *
 *  @{
 */
/* @brief heap datatype */
typedef struct xbt_heap *xbt_heap_t;

XBT_PUBLIC(xbt_heap_t) xbt_heap_new(int num, void_f_pvoid_t const free_func);
XBT_PUBLIC(void) xbt_heap_free(xbt_heap_t H);
XBT_PUBLIC(int) xbt_heap_size(xbt_heap_t H);

XBT_PUBLIC(void) xbt_heap_push(xbt_heap_t H, void *content, double key);
XBT_PUBLIC(void *) xbt_heap_pop(xbt_heap_t H);

XBT_PUBLIC(double) xbt_heap_maxkey(xbt_heap_t H);
XBT_PUBLIC(void *) xbt_heap_maxcontent(xbt_heap_t H);
XBT_PUBLIC(void) xbt_heap_set_update_callback(xbt_heap_t H, void (*update_callback)(void*, int));
XBT_PUBLIC(void*) xbt_heap_remove(xbt_heap_t H, int i);

/* @} */
#endif /* _XBT_HEAP_H */
