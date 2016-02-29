/* Copyright (c) 2012-2014. The SimGrid Team.
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
} *xbt_dynar_iterator_t;
typedef struct xbt_dynar_iterator_struct xbt_dynar_iterator_s;

/* Iterator methods */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int));
void xbt_dynar_iterator_reset(xbt_dynar_iterator_t it);
void xbt_dynar_iterator_seek(xbt_dynar_iterator_t it, int pos);
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it);
void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it);

/* Iterator generators */
xbt_dynar_t forward_indices_list(int size);
xbt_dynar_t reverse_indices_list(int size);
xbt_dynar_t random_indices_list(int size);

/* Shuffle */
/**************************************/
void xbt_dynar_shuffle_in_place(xbt_dynar_t indices_list);

#define xbt_dynar_swap_elements(d, type, i, j) \
  type tmp; \
  tmp = xbt_dynar_get_as(indices_list, (unsigned int)j, type); \
  xbt_dynar_set_as(indices_list, (unsigned int)j, type, \
    xbt_dynar_get_as(indices_list, (unsigned int)i, type)); \
  xbt_dynar_set_as(indices_list, (unsigned int)i, type, tmp);

#endif /* ITERATOR_H */
