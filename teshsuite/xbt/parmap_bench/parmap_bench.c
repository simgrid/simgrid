/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <xbt/dynar.h>
#include <xbt/parmap.h>
#include <xbt/sysdep.h>
#include "src/internal_config.h"        /* HAVE_FUTEX_H */
#include "simgrid/simix.h"
#include "xbt/xbt_os_time.h"

#define MODES_DEFAULT 0x7
#define TIMEOUT 10.0
#define ARRAY_SIZE 10007
#define FIBO_MAX 25

void (*fun_to_apply)(void *);

static const char *parmap_mode_name(e_xbt_parmap_mode_t mode)
{
  static char name[80];
  switch (mode) {
  case XBT_PARMAP_POSIX:
    snprintf(name, sizeof name, "POSIX");
    break;
  case XBT_PARMAP_FUTEX:
    snprintf(name, sizeof name, "FUTEX");
    break;
  case XBT_PARMAP_BUSY_WAIT:
    snprintf(name, sizeof name, "BUSY_WAIT");
    break;
  case XBT_PARMAP_DEFAULT:
    snprintf(name, sizeof name, "DEFAULT");
    break;
  default:
    snprintf(name, sizeof name, "UNKNOWN(%d)", (int)mode);
    break;
  }
  return name;
}

static int parmap_skip_mode(e_xbt_parmap_mode_t mode)
{
  switch (mode) {
#if !HAVE_FUTEX_H
  case XBT_PARMAP_FUTEX:
    printf("not available\n");
    return 1;
#endif
  default:
    return 0;
  }
}

static unsigned fibonacci(unsigned n)
{
  if (n < 2)
    return n;
  else
    return fibonacci(n - 1) + fibonacci(n - 2);
}

static void fun_small_comp(void *arg)
{
  unsigned *u = arg;
  *u = 2 * *u + 1;
}

static void fun_big_comp(void *arg)
{
  unsigned *u = arg;
  *u = fibonacci(*u % FIBO_MAX);
}

static void array_new(unsigned **a, xbt_dynar_t *data)
{
  int i;
  *a = xbt_malloc(ARRAY_SIZE * sizeof **a);
  *data = xbt_dynar_new(sizeof *a, NULL);
  xbt_dynar_shrink(*data, ARRAY_SIZE);
  for (i = 0 ; i < ARRAY_SIZE ; i++) {
    (*a)[i] = i;
    xbt_dynar_push_as(*data, void*, &(*a)[i]);
  }
}

static void bench_parmap_full(int nthreads, e_xbt_parmap_mode_t mode)
{
  unsigned *a;
  xbt_dynar_t data;
  xbt_parmap_t parmap;
  int i;
  double start_time, elapsed_time;

  printf("** mode = %-15s ", parmap_mode_name(mode));
  fflush(stdout);

  if (parmap_skip_mode(mode))
    return;

  array_new(&a, &data);

  i = 0;
  start_time = xbt_os_time();
  do {
    parmap = xbt_parmap_new(nthreads, mode);
    xbt_parmap_apply(parmap, fun_to_apply, data);
    xbt_parmap_destroy(parmap);
    elapsed_time = xbt_os_time() - start_time;
    i++;
  } while (elapsed_time < TIMEOUT);

  printf("ran %d times in %g seconds (%g/s)\n", i, elapsed_time, i / elapsed_time);

  xbt_dynar_free(&data);
  xbt_free(a);
}

static void bench_parmap_apply(int nthreads, e_xbt_parmap_mode_t mode)
{
  unsigned *a;
  xbt_dynar_t data;
  double start_time, elapsed_time;

  printf("** mode = %-15s ", parmap_mode_name(mode));
  fflush(stdout);

  if (parmap_skip_mode(mode))
    return;

  array_new(&a, &data);

  xbt_parmap_t parmap = xbt_parmap_new(nthreads, mode);
  int i = 0;
  start_time = xbt_os_time();
  do {
    xbt_parmap_apply(parmap, fun_to_apply, data);
    elapsed_time = xbt_os_time() - start_time;
    i++;
  } while (elapsed_time < TIMEOUT);
  xbt_parmap_destroy(parmap);

  printf("ran %d times in %g seconds (%g/s)\n",
         i, elapsed_time, i / elapsed_time);

  xbt_dynar_free(&data);
  xbt_free(a);
}

static void bench_all_modes(void (*bench_fun)(int, e_xbt_parmap_mode_t),
                            int nthreads, unsigned modes)
{
  e_xbt_parmap_mode_t all_modes[] = {XBT_PARMAP_POSIX, XBT_PARMAP_FUTEX, XBT_PARMAP_BUSY_WAIT, XBT_PARMAP_DEFAULT};

  for (unsigned i = 0 ; i < sizeof all_modes / sizeof all_modes[0] ; i++) {
    if (1U << i & modes)
      bench_fun(nthreads, all_modes[i]);
  }
}

int main(int argc, char *argv[])
{
  int nthreads;
  unsigned modes = MODES_DEFAULT;

  SIMIX_global_init(&argc, argv);

  if (argc != 2 && argc != 3) {
    fprintf(stderr, "Usage: %s nthreads [modes]\n"
            "    nthreads - number of working threads\n"
            "    modes    - bitmask of modes to test\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  nthreads = atoi(argv[1]);
  if (nthreads < 1) {
    fprintf(stderr, "ERROR: invalid thread count: %d\n", nthreads);
    return EXIT_FAILURE;
  }
  if (argc == 3)
    modes = strtol(argv[2], NULL, 0);

  printf("Parmap benchmark with %d workers (modes = %#x)...\n\n", nthreads, modes);

  fun_to_apply = fun_small_comp;

  printf("Benchmark for parmap create+apply+destroy (small comp):\n");
  bench_all_modes(bench_parmap_full, nthreads, modes);
  printf("\n");

  printf("Benchmark for parmap apply only (small comp):\n");
  bench_all_modes(bench_parmap_apply, nthreads, modes);
  printf("\n");

  fun_to_apply = fun_big_comp;

  printf("Benchmark for parmap create+apply+destroy (big comp):\n");
  bench_all_modes(bench_parmap_full, nthreads, modes);
  printf("\n");

  printf("Benchmark for parmap apply only (big comp):\n");
  bench_all_modes(bench_parmap_apply, nthreads, modes);
  printf("\n");

  return EXIT_SUCCESS;
}
