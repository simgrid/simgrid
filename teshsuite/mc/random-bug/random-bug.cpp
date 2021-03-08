/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(random_bug, "For this example");

enum class Behavior { ABORT, ASSERT, PRINTF, SEGV };

Behavior behavior;

/** A fake application with a bug occurring for some random values */
static void app()
{
  int x = MC_random(0, 5);
  int y = MC_random(0, 5);
  XBT_DEBUG("got %d %d", x, y);

  if (behavior == Behavior::ASSERT) {
    MC_assert(x != 3 || y != 4);
  } else if (behavior == Behavior::PRINTF) {
    if (x == 3 && y == 4)
      XBT_ERROR("Error reached");
  } else if (behavior == Behavior::ABORT) {
    if (x == 3 && y == 4)
      abort();
  } else if (behavior == Behavior::SEGV) {
    int* A = 0;
    if (x == 3 && y == 4)
      *A = 1;
  } else {
    DIE_IMPOSSIBLE;
  }
}

/** Main function */
int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: random-bug abort|assert|printf|segv <platformfile>");
  if (strcmp(argv[1], "abort") == 0) {
    XBT_INFO("Behavior: abort");
    behavior = Behavior::ABORT;
  } else if (strcmp(argv[1], "assert") == 0) {
    XBT_INFO("Behavior: assert");
    behavior = Behavior::ASSERT;
  } else if (strcmp(argv[1], "printf") == 0) {
    XBT_INFO("Behavior: printf");
    behavior = Behavior::PRINTF;
  } else if (strcmp(argv[1], "segv") == 0) {
    XBT_INFO("Behavior: segv");
    behavior = Behavior::SEGV;
  } else {
    xbt_die("Please use either 'abort', 'assert', 'printf', or 'segv' as first parameter,"
            " to specify what to do when the error is found.");
  }

  e.load_platform(argv[2]);

  simgrid::s4u::Actor::create("app", simgrid::s4u::Host::by_name("Fafard"), &app);
  e.run();
}
