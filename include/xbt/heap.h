/* Copyright (c) 2004-2007, 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_H
#define _XBT_HEAP_H

#include "xbt/misc.h"
#include "xbt/dynar.h"          /* void_f_pvoid_t */

SG_BEGIN_DECL()

/** @addtogroup XBT_heap
 *  @brief This section describes the API to generic heap with O(log(n)) access.
 *
 *  If you are using C++ you might want to use std::priority_queue instead.
 *
 *  @{
 */
/* @brief heap datatype */
typedef struct xbt_heap *xbt_heap_t;

XBT_PUBLIC(xbt_heap_t) xbt_heap_new(int init_size, void_f_pvoid_t const free_func);
XBT_PUBLIC(void) xbt_heap_free(xbt_heap_t H);
XBT_PUBLIC(int) xbt_heap_size(xbt_heap_t H);

XBT_PUBLIC(void) xbt_heap_push(xbt_heap_t H, void *content, double key);
XBT_PUBLIC(void *) xbt_heap_pop(xbt_heap_t H);
XBT_PUBLIC(void) xbt_heap_rm_elm(xbt_heap_t H, void *content, double key);

XBT_PUBLIC(double) xbt_heap_maxkey(xbt_heap_t H);
XBT_PUBLIC(void *) xbt_heap_maxcontent(xbt_heap_t H);
XBT_PUBLIC(void) xbt_heap_set_update_callback(xbt_heap_t H, void (*update_callback) (void*, int));
XBT_PUBLIC(void *) xbt_heap_remove(xbt_heap_t H, int i);
XBT_PUBLIC(void ) xbt_heap_update(xbt_heap_t H, int i, double key);

/* @} */
SG_END_DECL()

#endif                          /* _XBT_HEAP_H */
