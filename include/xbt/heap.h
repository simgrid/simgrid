/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_H
#define _XBT_HEAP_H

#include "xbt/dynar.h"

typedef struct xbt_heap *xbt_heap_t;

/* The following two definitions concern the type of the keys used for
   the heaps. That should be handled via configure (FIXME). */
typedef long double xbt_heap_float_t;
#define XBT_HEAP_FLOAT_T "%Lg"	/* for printing purposes */

/* /\* pointer to a function freeing something (should be common to all .h : FIXME) *\/ */
/* typedef void (void_f_pvoid_t) (void *); */

xbt_heap_t xbt_heap_new(int num, void_f_pvoid_t free_func);
void xbt_heap_free(xbt_heap_t H);
int xbt_heap_size(xbt_heap_t H);

void xbt_heap_push(xbt_heap_t H, void *content, xbt_heap_float_t key);
void *xbt_heap_pop(xbt_heap_t H);

xbt_heap_float_t xbt_heap_maxkey(xbt_heap_t H);
void *xbt_heap_maxcontent(xbt_heap_t H);

#endif				/* _XBT_HEAP_H */
