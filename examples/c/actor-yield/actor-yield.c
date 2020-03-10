/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/str.h"

/* This example does not much: It just spans over-polite actors that yield a large amount
 * of time before ending.
 *
 * This serves as an example for the sg_actor_yield() function, with which an actor can request
 * to be rescheduled after the other actors that are ready at the current timestamp.
 *
 * It can also be used to benchmark our context-switching mechanism.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(actor_yield, "Messages specific for this example");

/* Main function of the Yielder actor */
static void yielder(int argc, char* argv[])
{
  xbt_assert(argc == 2, "The sender function expects 1 arguments from the XML deployment file");
  long number_of_yields = xbt_str_parse_int(argv[1], "Invalid amount of yields: %s"); /* - number of yields */

  for (int i = 0; i < number_of_yields; i++)
    sg_actor_yield();
  XBT_INFO("I yielded %ld times. Goodbye now!", number_of_yields);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */

  simgrid_register_function("yielder", yielder);
  simgrid_load_deployment(argv[2]); /* - Deploy the sender and receiver actors */

  simgrid_run();
  return 0;
}
