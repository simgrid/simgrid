/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/msg.h>
#include <simgrid/modelchecker.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(random_bug, "Application");

/** An (fake) application with a bug occuring for some random values */
static int app(int argc, char *argv[])
{
  int x = MC_random(0, 5);
  int y = MC_random(0, 5);

  if (MC_is_active()) {
    MC_assert(x !=3 || y !=4);
  }
  if (x ==3 && y ==4) {
    fprintf(stderr, "Error reached\n");
  }

  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  MSG_function_register("app", &app);
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);
  return MSG_main();
}
