/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_HEAP_H
#define _XBT_HEAP_H

#include "xbt/misc.h"

typedef struct xbt_heap *xbt_heap_t;

xbt_heap_t xbt_heap_new(int num, void_f_pvoid_t free_func);
void xbt_heap_free(xbt_heap_t H);
int xbt_heap_size(xbt_heap_t H);

void xbt_heap_push(xbt_heap_t H, void *content, double key);
void *xbt_heap_pop(xbt_heap_t H);

double xbt_heap_maxkey(xbt_heap_t H);
void *xbt_heap_maxcontent(xbt_heap_t H);

#endif				/* _XBT_HEAP_H */
