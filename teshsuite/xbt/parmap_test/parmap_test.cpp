/* parmap_test -- test parmap                                               */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h" // HAVE_FUTEX_H
#include <simgrid/s4u/Engine.hpp>
#include <xbt.h>
#include <xbt/parmap.hpp>

#include <algorithm>
#include <cstdlib>
#include <numeric> // std::iota
#include <sstream>
#include <thread>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(parmap_test, "Test for parmap");

static void fun_double(unsigned* arg)
{
  *arg = 2 * *arg + 1;
}

static int test_parmap_basic(e_xbt_parmap_mode_t mode)
{
  int ret = 0;
  for (unsigned num_workers = 1; num_workers <= 16; num_workers *= 2) {
    const unsigned len = 1033;
    const unsigned num = 5;

    simgrid::xbt::Parmap<unsigned*> parmap(num_workers, mode);
    std::vector<unsigned> a(len);
    std::vector<unsigned*> data(len);
    std::iota(begin(a), end(a), 0);
    std::iota(begin(data), end(data), &a[0]);

    for (unsigned i = 0; i < num; i++)
      parmap.apply(fun_double, data);

    for (unsigned i = 0; i < len; i++) {
      unsigned expected = (1U << num) * (i + 1) - 1;
      if (a[i] != expected) {
        XBT_CRITICAL("with %u threads, a[%u]: expected %u, got %u", num_workers, i, expected, a[i]);
        ret = 1;
        break;
      }
    }
  }
  return ret;
}

static void fun_get_id(std::string* arg)
{
  std::stringstream ss;
  ss << std::this_thread::get_id();
  *arg = ss.str();
  xbt_os_sleep(0.05);
}

static int test_parmap_extended(e_xbt_parmap_mode_t mode)
{
  int ret = 0;

  for (unsigned num_workers = 1; num_workers <= 16; num_workers *= 2) {
    const unsigned len = 2 * num_workers;

    simgrid::xbt::Parmap<std::string*> parmap(num_workers, mode);
    std::vector<std::string> a(len);
    std::vector<std::string*> data(len);
    std::iota(begin(data), end(data), &a[0]);

    parmap.apply(fun_get_id, data);

    std::sort(begin(a), end(a));
    unsigned count = std::distance(begin(a), std::unique(begin(a), end(a)));
    if (count != num_workers) {
      XBT_CRITICAL("only %u/%u threads did some work", count, num_workers);
      ret = 1;
    }
  }
  return ret;
}

int main(int argc, char** argv)
{
  int status = 0;
  xbt_log_control_set("parmap_test.fmt:[%c/%p]%e%m%n");
  simgrid::s4u::Engine e(&argc, argv);
  SIMIX_context_set_nthreads(16); // dummy value > 1

  XBT_INFO("Basic testing posix");
  status += test_parmap_basic(XBT_PARMAP_POSIX);
  XBT_INFO("Basic testing futex");
#if HAVE_FUTEX_H
  status += test_parmap_basic(XBT_PARMAP_FUTEX);
#endif
  XBT_INFO("Basic testing busy wait");
  status += test_parmap_basic(XBT_PARMAP_BUSY_WAIT);

  XBT_INFO("Extended testing posix");
  status += test_parmap_extended(XBT_PARMAP_POSIX);
  XBT_INFO("Extended testing futex");
#if HAVE_FUTEX_H
  status += test_parmap_extended(XBT_PARMAP_FUTEX);
#endif
  XBT_INFO("Extended testing busy wait");
  status += test_parmap_extended(XBT_PARMAP_BUSY_WAIT);

  return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
