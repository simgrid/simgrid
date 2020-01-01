/* stack_overflow -- simple program generating a stack overflow             */

/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.hpp"
#include "xbt/log.h"
#include <simgrid/engine.h>

#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "my log messages");

static unsigned collatz(unsigned c0, unsigned n)
{
  unsigned x;
  if (n == 0) {
    x = c0;
  } else {
    x = collatz(c0, n - 1);
    if (x % 2 == 0)
      x = x / 2;
    else
      x = 3 * x + 1;
  }
  return x;
}

static void master()
{
  XBT_INFO("Launching our nice bugged recursive function...");
  unsigned i = 1;
  while (i <= 0x80000000U) {
    i *= 2;
    unsigned res = collatz(i, i);
    XBT_VERB("collatz(%u, %u) returned %u", i, i, res);
  }
}

int main(int argc, char* argv[])
{
  SIMIX_global_init(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform.xml\n", argv[0]);

  simgrid_load_platform(argv[1]);
  simcall_process_create("master", master, NULL, sg_host_by_name("Tremblay"), NULL);
  SIMIX_run();

  return 0;
}
