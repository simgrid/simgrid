/* stack_overflow -- simple program generating a stack overflow          */

/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/platf.h"
#include "simgrid/simix.h"
#include "xbt/log.h"

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

static int master(int argc, char *argv[])
{
  XBT_INFO("Launching our nice bugged recursive function...");
  unsigned i = 1;
  do {
    i *= 2;
    unsigned res = collatz(i, i);
    XBT_VERB("collatz(%u, %u) returned %u", i, i, res);
  } while (i <= 0x80000000u);
  return 0;
}

int main(int argc, char *argv[])
{
  SIMIX_global_init(&argc, argv);

  if (argc != 3) {
    printf("Usage: %s platform.xml deployment.xml\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  SIMIX_function_register("master", master);
  SIMIX_create_environment(argv[1]);
  SIMIX_launch_application(argv[2]);
  SIMIX_run();

  return 0;
}
