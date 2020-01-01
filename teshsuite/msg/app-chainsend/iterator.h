/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef ITERATOR_H
#define ITERATOR_H

#include "xbt/dynar.h"
#include "xbt/sysdep.h"

/* Random iterator for xbt_dynar */
typedef struct xbt_dynar_iterator_struct {
  xbt_dynar_t list;
  xbt_dynar_t indices_list;
  int current;
  unsigned long length;
  xbt_dynar_t (*criteria_fn)(int size);
} * xbt_dynar_iterator_t;
typedef struct xbt_dynar_iterator_struct xbt_dynar_iterator_s;

/* Iterator methods */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int));
void* xbt_dynar_iterator_next(xbt_dynar_iterator_t it);
void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it);

/* Iterator generators */
xbt_dynar_t forward_indices_list(int size);

#endif /* ITERATOR_H */
