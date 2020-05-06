/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific for this example");

/* Executed on actor termination*/
static void my_onexit(XBT_ATTRIB_UNUSED int ignored1, XBT_ATTRIB_UNUSED void* ignored2)
{
  XBT_INFO("Exiting now (done sleeping or got killed)."); /* - Just display an informative message (see tesh file) */
}

/* Just sleep until termination */
static void sleeper(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_actor_on_exit(my_onexit, NULL);

  XBT_INFO("Hello! I go to sleep.");
  sg_actor_sleep_for(10);
  XBT_INFO("Done sleeping.");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */
  simgrid_register_function("sleeper", sleeper);
  simgrid_load_deployment(argv[2]); /* - Deploy the sleeper actors with explicit start/kill times */

  simgrid_run(); /* - Run the simulation */

  return 0;
}
