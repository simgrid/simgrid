/* Copyright (c) 2014-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

#include <stdio.h> /* snprintf */

enum { ABORT, ASSERT, PRINTF } behavior;

/** An (fake) application with a bug occuring for some random values */
static void app()
{
  int x = MC_random(0, 5);
  int y = MC_random(0, 5);

  if (behavior == ABORT) {
    abort();
  } else if (behavior == ASSERT) {
    MC_assert(x != 3 || y != 4);
  } else if (behavior == PRINTF) {
    if (x == 3 && y == 4)
      fprintf(stderr, "Error reached\n");
  }
}

/** Main function */
int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: random-bug raise|assert <platformfile>");
  if (strcmp(argv[1], "abort") == 0) {
    printf("Behavior: abort\n");
    behavior = ABORT;
  } else if (strcmp(argv[1], "assert") == 0) {
    printf("Behavior: assert\n");
    behavior = ASSERT;
  } else if (strcmp(argv[1], "printf") == 0) {
    printf("Behavior: printf\n");
    behavior = PRINTF;
  }

  e.load_platform(argv[2]);

  simgrid::s4u::Actor::create("app", simgrid::s4u::Host::by_name("Fafard"), &app);
  e.run();
}
