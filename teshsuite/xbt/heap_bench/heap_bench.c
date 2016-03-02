/* A few tests for the xbt_heap module                                      */

/* Copyright (c) 2004-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <xbt/xbt_os_time.h>

#include "xbt/heap.h"
#include "xbt/sysdep.h"         /* calloc, printf */

#define MAX_TEST 1000000

static int compare_double(const void *a, const void *b)
{
  double pa = *((double *) a);
  double pb = *((double *) b);

  if (pa > pb)
    return 1;
  if (pa == pb)
    return 0;
  return -1;
}

static void test_reset_heap(xbt_heap_t * heap, int size)
{
  xbt_heap_free(*heap);
  *heap = xbt_heap_new(size, NULL);

  for (int i = 0; i < size; i++) {
    xbt_heap_push(*heap, NULL, (10.0 * rand() / (RAND_MAX + 1.0)));
  }
}

static void test_heap_validity(int size)
{
  xbt_heap_t heap = xbt_heap_new(size, NULL);
  double *tab = xbt_new0(double, size);
  int i;

  for (i = 0; i < size; i++) {
    tab[i] = (double) (10.0 * rand() / (RAND_MAX + 1.0));
    xbt_heap_push(heap, NULL, (double) tab[i]);
  }

  qsort(tab, size, sizeof(double), compare_double);

  for (i = 0; i < size; i++) {
    /*     printf("%g" " ", xbt_heap_maxkey(heap)); */
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

static void test_heap_mean_operation(int size)
{
  xbt_heap_t heap = xbt_heap_new(size, NULL);

  double date = xbt_os_time() * 1000000;
  for (int i = 0; i < size; i++)
    xbt_heap_push(heap, NULL, (10.0 * rand() / (RAND_MAX + 1.0)));

  date = xbt_os_time() * 1000000 - date;
  printf("Creation time  %d size heap : %g\n", size, date);

  date = xbt_os_time() * 1000000;
  for (int j = 0; j < MAX_TEST; j++) {

    if (!(j % size) && j)
      test_reset_heap(&heap, size);

    double val = xbt_heap_maxkey(heap);
    xbt_heap_pop(heap);
    xbt_heap_push(heap, NULL, 3.0 * val);
  }
  date = xbt_os_time() * 1000000 - date;
  printf("Mean access time for a %d size heap : %g\n", size, date * 1.0 / (MAX_TEST + 0.0));

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
