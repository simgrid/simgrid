//---------------------------------------------------------------------------

#pragma hdrstop

/*#include "..\..\..\..\..\..\include\xbt\dict.h"*/

/* A few tests for the xbt_heap module                                      */

/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>

#include "xbt/heap.h"
#include "gras/virtu.h"         /* time manipulation in bench */

#define MAX_TEST 1000000

int _XBT_CALL compare_double(const void *a, const void *b);
void test_heap_validity(int size);
void test_heap_mean_operation(int size);
void test_reset_heap(xbt_heap_t heap, int size);


int _XBT_CALL compare_double(const void *a, const void *b)
{
  double pa, pb;

  pa = *((double *) a);
  pb = *((double *) b);

  if (pa > pb)
    return 1;

  if (pa == pb)
    return 0;

  return -1;
}

void test_heap_validity(int size)
{
  xbt_heap_t heap = xbt_heap_new(size, NULL);
  double *tab = calloc(size, sizeof(double));
  int i;

  for (i = 0; i < size; i++) {
    tab[i] = (10.0 * rand() / (RAND_MAX + 1.0));
    xbt_heap_push(heap, NULL, tab[i]);
  }

  qsort(tab, size, sizeof(double), compare_double);

  for (i = 0; i < size; i++) {
    /*     printf("%lg" " ", xbt_heap_maxkey(heap)); */
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
  double val;
  double date = 0;
  int i, j;

  date = gras_os_time() * 1000000;

  for (i = 0; i < size; i++)
    xbt_heap_push(heap, NULL, (10.0 * rand() / (RAND_MAX + 1.0)));

  date = gras_os_time() * 1000000 - date;

  printf("Creation time  %d size heap : %g\n", size, date);

  date = gras_os_time() * 1000000;

  for (j = 0; j < MAX_TEST; j++) {

    if (!(j % size) && j)
      test_reset_heap(heap, size);

    val = xbt_heap_maxkey(heap);
    xbt_heap_pop(heap);
    xbt_heap_push(heap, NULL, 3.0 * val);
  }

  date = gras_os_time() * 1000000 - date;
  printf("Mean access time for a %d size heap : %g\n", size,
         date * 1.0 / (MAX_TEST + 0.0));

  xbt_heap_free(heap);
}

void test_reset_heap(xbt_heap_t heap, int size)
{
  int i;
  xbt_heap_free(heap);
  heap = xbt_heap_new(size, NULL);

  for (i = 0; i < size; i++) {
    xbt_heap_push(heap, NULL, (10.0 * rand() / (RAND_MAX + 1.0)));
  }
}


#pragma argsused

int main(int argc, char **argv)
{
  int size;

  for (size = 100; size < 10000; size *= 10) {
    test_heap_validity(size);
    test_heap_mean_operation(size);
  }
  return 0;
}

//---------------------------------------------------------------------------
