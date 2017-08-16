/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h" // HAVE_FUTEX_H
#include <simgrid/msg.h>
#include <xbt.h>
#include <xbt/parmap.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric> // std::iota
#include <string>
#include <vector>

#define MODES_DEFAULT 0x7
#define TIMEOUT 10.0
#define ARRAY_SIZE 10007
#define FIBO_MAX 25

void (*fun_to_apply)(unsigned*);

static std::string parmap_mode_name(e_xbt_parmap_mode_t mode)
{
  std::string name;
  switch (mode) {
    case XBT_PARMAP_POSIX:
      name = "POSIX";
      break;
    case XBT_PARMAP_FUTEX:
      name = "FUTEX";
      break;
    case XBT_PARMAP_BUSY_WAIT:
      name = "BUSY_WAIT";
      break;
    case XBT_PARMAP_DEFAULT:
      name = "DEFAULT";
      break;
    default:
      name = "UNKNOWN(" + std::to_string(mode) + ")";
      break;
  }
  return name;
}

static bool parmap_skip_mode(e_xbt_parmap_mode_t mode)
{
  if (mode == XBT_PARMAP_FUTEX && not HAVE_FUTEX_H) {
    std::cout << "not available\n";
    return true;
  } else {
    return false;
  }
}

static unsigned fibonacci(unsigned n)
{
  if (n < 2)
    return n;
  else
    return fibonacci(n - 1) + fibonacci(n - 2);
}

static void fun_small_comp(unsigned* arg)
{
  *arg = 2 * *arg + 1;
}

static void fun_big_comp(unsigned* arg)
{
  *arg = fibonacci(*arg % FIBO_MAX);
}

static void bench_parmap_full(int nthreads, e_xbt_parmap_mode_t mode)
{
  std::cout << "** mode = " << std::left << std::setw(15) << parmap_mode_name(mode) << " ";
  std::cout.flush();

  if (parmap_skip_mode(mode))
    return;

  std::vector<unsigned> a(ARRAY_SIZE);
  std::vector<unsigned*> data(ARRAY_SIZE);
  std::iota(begin(a), end(a), 0);
  std::iota(begin(data), end(data), &a[0]);

  int i             = 0;
  double start_time = xbt_os_time();
  double elapsed_time;
  do {
    {
      simgrid::xbt::Parmap<unsigned*> parmap(nthreads, mode);
      parmap.apply(fun_to_apply, data);
    } // enclosing block to ensure that the parmap is destroyed here.
    elapsed_time = xbt_os_time() - start_time;
    i++;
  } while (elapsed_time < TIMEOUT);

  std::cout << "ran " << i << " times in " << elapsed_time << " seconds (" << (i / elapsed_time) << "/s)\n";
}

static void bench_parmap_apply(int nthreads, e_xbt_parmap_mode_t mode)
{
  std::cout << "** mode = " << std::left << std::setw(15) << parmap_mode_name(mode) << " ";
  std::cout.flush();

  if (parmap_skip_mode(mode))
    return;

  std::vector<unsigned> a(ARRAY_SIZE);
  std::vector<unsigned*> data(ARRAY_SIZE);
  std::iota(begin(a), end(a), 0);
  std::iota(begin(data), end(data), &a[0]);

  simgrid::xbt::Parmap<unsigned*> parmap(nthreads, mode);
  int i             = 0;
  double start_time = xbt_os_time();
  double elapsed_time;
  do {
    parmap.apply(fun_to_apply, data);
    elapsed_time = xbt_os_time() - start_time;
    i++;
  } while (elapsed_time < TIMEOUT);

  std::cout << "ran " << i << " times in " << elapsed_time << " seconds (" << (i / elapsed_time) << "/s)\n";
}

static void bench_all_modes(void (*bench_fun)(int, e_xbt_parmap_mode_t), int nthreads, unsigned modes)
{
  std::vector<e_xbt_parmap_mode_t> all_modes = {XBT_PARMAP_POSIX, XBT_PARMAP_FUTEX, XBT_PARMAP_BUSY_WAIT,
                                                XBT_PARMAP_DEFAULT};

  for (unsigned i = 0; i < all_modes.size(); i++) {
    if (1U << i & modes)
      bench_fun(nthreads, all_modes[i]);
  }
}

int main(int argc, char* argv[])
{
  int nthreads;
  unsigned modes = MODES_DEFAULT;

  MSG_init(&argc, argv);

  if (argc != 2 && argc != 3) {
    std::cerr << "Usage: " << argv[0] << " nthreads [modes]\n"
              << "    nthreads - number of working threads\n"
              << "    modes    - bitmask of modes to test\n";
    return EXIT_FAILURE;
  }
  nthreads = atoi(argv[1]);
  if (nthreads < 1) {
    std::cerr << "ERROR: invalid thread count: " << nthreads << "\n";
    return EXIT_FAILURE;
  }
  if (argc == 3)
    modes = strtol(argv[2], NULL, 0);

  std::cout << "Parmap benchmark with " << nthreads << " workers (modes = " << std::hex << modes << std::dec
            << ")...\n\n";

  fun_to_apply = &fun_small_comp;

  std::cout << "Benchmark for parmap create+apply+destroy (small comp):\n";
  bench_all_modes(bench_parmap_full, nthreads, modes);
  std::cout << std::endl;

  std::cout << "Benchmark for parmap apply only (small comp):\n";
  bench_all_modes(bench_parmap_apply, nthreads, modes);
  std::cout << std::endl;

  fun_to_apply = &fun_big_comp;

  std::cout << "Benchmark for parmap create+apply+destroy (big comp):\n";
  bench_all_modes(bench_parmap_full, nthreads, modes);
  std::cout << std::endl;

  std::cout << "Benchmark for parmap apply only (big comp):\n";
  bench_all_modes(bench_parmap_apply, nthreads, modes);
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
