/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(random_bug, "For this example");

enum class Behavior { ABORT, ASSERT, PRINTF };

Behavior behavior;

/** A fake application with a bug occurring for some random values */
static void app()
{
  int x = MC_random(0, 5);
  int y = MC_random(0, 5);

  if (behavior == Behavior::ASSERT) {
    MC_assert(x != 3 || y != 4);
  } else if (behavior == Behavior::PRINTF) {
    if (x == 3 && y == 4)
      XBT_ERROR("Error reached");
  } else { // behavior == Behavior::ABORT
    abort();
  }
}

/** Main function */
int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: random-bug raise|assert <platformfile>");
  if (strcmp(argv[1], "abort") == 0) {
    XBT_INFO("Behavior: abort");
    behavior = Behavior::ABORT;
  } else if (strcmp(argv[1], "assert") == 0) {
    XBT_INFO("Behavior: assert");
    behavior = Behavior::ASSERT;
  } else if (strcmp(argv[1], "printf") == 0) {
    XBT_INFO("Behavior: printf");
    behavior = Behavior::PRINTF;
  } else {
    xbt_die("Please use either 'abort', 'assert' or 'printf' as first parameter, to specify what to do when the error "
            "is found.");
  }

  e.load_platform(argv[2]);

  simgrid::s4u::Actor::create("app", simgrid::s4u::Host::by_name("Fafard"), &app);
  e.run();
}
