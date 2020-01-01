/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "iterator.h"

/* http://stackoverflow.com/a/3348142 */
/**************************************/

/* Allocates and initializes a new xbt_dynar iterator for list, using criteria_fn as iteration criteria
   criteria_fn: given an array size, it must generate a list containing the indices of every item in some order */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int))
{
  xbt_dynar_iterator_t it = xbt_new(xbt_dynar_iterator_s, 1);

  it->list         = list;
  it->length       = xbt_dynar_length(list);
  it->indices_list = criteria_fn(it->length); // Creates and fills a dynar of int
  it->criteria_fn  = criteria_fn;
  it->current      = 0;

  return it;
}

/* Returns the next element iterated by iterator it, NULL if there are no more elements */
void* xbt_dynar_iterator_next(xbt_dynar_iterator_t it)
{
  if (it->current >= it->length) {
    return NULL;
  } else {
    const int* next = xbt_dynar_get_ptr(it->indices_list, it->current);
    it->current++;
    return xbt_dynar_get_ptr(it->list, *next);
  }
}

void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  xbt_free_ref(&it);
}

xbt_dynar_t forward_indices_list(int size)
{
  xbt_dynar_t indices_list = xbt_dynar_new(sizeof(int), NULL);
  for (int i = 0; i < size; i++)
    xbt_dynar_push_as(indices_list, int, i);
  return indices_list;
}
