/* $Id$ */

/* A few tests for the xbt_heap module                                      */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>

#include "xbt/heap.h"
#include "gras/virtu.h" /* time manipulation in bench */

#define MAX_TEST 1000000

int compare_xbt_heap_float_t(const void *a, const void *b);
void test_heap_validity(int size);
void test_heap_mean_operation(int size);

int compare_xbt_heap_float_t(const void *a, const void *b)
{
  xbt_heap_float_t pa, pb;

  pa = *((xbt_heap_float_t *) a);
  pb = *((xbt_heap_float_t *) b);

  if (pa > pb)
    return 1;
  if (pa == pb)
    return 0;
  return -1;
}

void test_heap_validity(int size)
{
  xbt_heap_t heap = xbt_heap_new(size, NULL);
  xbt_heap_float_t *tab = calloc(size, sizeof(xbt_heap_float_t));
  int i;

  for (i = 0; i < size; i++) {
    tab[i] = (10.0 * rand() / (RAND_MAX + 1.0));
    xbt_heap_push(heap, NULL, tab[i]);
  }

  qsort(tab, size, sizeof(xbt_heap_float_t), compare_xbt_heap_float_t);

  for (i = 0; i < size; i++) {
    /*     printf(XBT_HEAP_FLOAT_T " ", xbt_heap_maxkey(heap)); */
    if (xbt_heap_maxkey(heap) != tab[i]) {
      fprintf(stderr, "Problem !\n");
      exit(1);
    }
    xbt_heap_pop(heap);
  }
  xbt_heap_free(heap);
  free(tab);
  printf("Validity test complete!\n");
}

void test_heap_mean_operation(int size)
{
  xbt_heap_t heap = xbt_heap_new(size, NULL);
  xbt_heap_float_t val;
  double date = 0;
  int i, j;

  date = gras_os_time() * 1000000;
  for (i = 0; i < size; i++)
    xbt_heap_push(heap, NULL, (10.0 * rand() / (RAND_MAX + 1.0)));
  date = gras_os_time() * 1000000 - date;
  printf("Creation time  %d size heap : %g\n", size, date);

  date = gras_os_time() * 1000000;
  for (j = 0; j < MAX_TEST; j++) {
    val = xbt_heap_maxkey(heap);
    xbt_heap_pop(heap);
    xbt_heap_push(heap, NULL, 3.0 * val);
  }
  date = gras_os_time() * 1000000 - date;
  printf("Mean access time for a %d size heap : %g\n", size,
	 date * 1.0 / (MAX_TEST + 0.0));

  xbt_heap_free(heap);
}

int main(int argc, char **argv)
{
  int size;
  for (size = 100; size < 10000; size *= 10) {
    test_heap_validity(size);
    test_heap_mean_operation(size);
  }
  return 0;
}
