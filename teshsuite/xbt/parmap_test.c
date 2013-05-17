/* parmap_test -- test parmap                                               */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.h"
#include "xbt.h"
#include "xbt/ex.h"
#include "xbt/xbt_os_time.h"
#include "internal_config.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(parmap_test, "Test for parmap");

static void fun_double(void *arg)
{
  unsigned *u = arg;
  *u = 2 * *u + 1;
}

static void test_parmap_basic(e_xbt_parmap_mode_t mode)
{
  unsigned num_workers;
  for (num_workers = 1 ; num_workers <= 16 ; num_workers *= 2) {
    const unsigned len = 1033;
    const unsigned num = 5;
    unsigned *a;
    xbt_dynar_t data;
    xbt_parmap_t parmap;
    unsigned i;

    parmap = xbt_parmap_new(num_workers, mode);

    a = xbt_malloc(len * sizeof *a);
    data = xbt_dynar_new(sizeof a, NULL);
    for (i = 0; i < len; i++) {
      a[i] = i;
      xbt_dynar_push_as(data, void *, &a[i]);
    }

    for (i = 0; i < num; i++)
      xbt_parmap_apply(parmap, fun_double, data);

    for (i = 0; i < len; i++) {
      unsigned expected = (1U << num) * (i + 1) - 1;
      xbt_test_assert(a[i] == expected,
                      "a[%u]: expected %u, got %u", i, expected, a[i]);
    }

    xbt_dynar_free(&data);
    xbt_free(a);
    xbt_parmap_destroy(parmap);
  }
}

static void fun_get_id(void *arg)
{
  *(uintptr_t *)arg = (uintptr_t)xbt_os_thread_self();
  xbt_os_sleep(0.5);
}

static int fun_compare(const void *pa, const void *pb)
{
  uintptr_t a = *(uintptr_t *)pa;
  uintptr_t b = *(uintptr_t *)pb;
  return a < b ? -1 : a > b ? 1 : 0;
}

static void test_parmap_extended(e_xbt_parmap_mode_t mode)
{
  unsigned num_workers;

  for (num_workers = 1 ; num_workers <= 16 ; num_workers *= 2) {
    const unsigned len = 2 * num_workers;
    uintptr_t *a;
    xbt_parmap_t parmap;
    xbt_dynar_t data;
    unsigned i;
    unsigned count;

    parmap = xbt_parmap_new(num_workers, mode);

    a = xbt_malloc(len * sizeof *a);
    data = xbt_dynar_new(sizeof a, NULL);
    for (i = 0; i < len; i++)
      xbt_dynar_push_as(data, void *, &a[i]);

    xbt_parmap_apply(parmap, fun_get_id, data);

    qsort(a, len, sizeof a[0], fun_compare);
    count = 1;
    for (i = 1; i < len; i++)
      if (a[i] != a[i - 1])
        count++;
    xbt_test_assert(count == num_workers,
                    "only %u/%u threads did some work", count, num_workers);

    xbt_dynar_free(&data);
    xbt_free(a);
    xbt_parmap_destroy(parmap);
  }
}

int main(int argc, char** argv)
{
  SIMIX_global_init(&argc, argv);

  XBT_INFO("Basic testing posix");
  test_parmap_basic(XBT_PARMAP_POSIX);
  XBT_INFO("Basic testing futex");
#ifdef HAVE_FUTEX_H
  test_parmap_basic(XBT_PARMAP_FUTEX);
#endif
  XBT_INFO("Basic testing busy wait");
  test_parmap_basic(XBT_PARMAP_BUSY_WAIT);

  XBT_INFO("Extended testing posix");
  test_parmap_extended(XBT_PARMAP_POSIX);
  XBT_INFO("Extended testing futex");
#ifdef HAVE_FUTEX_H
  test_parmap_extended(XBT_PARMAP_FUTEX);
#endif
  XBT_INFO("Extended testing busy wait");
  test_parmap_extended(XBT_PARMAP_BUSY_WAIT);

  return 0;
}
