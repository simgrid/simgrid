/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

/* This example does not much: It just spans over-polite processes that yield a large amount
 * of time before ending.
 *
 * This serves as an example for the MSG_process_yield() function, with which a process can request
 * to be rescheduled after the other processes that are ready at the current timestamp.
 *
 * It can also be used to benchmark our context-switching mechanism.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_yield, "Messages specific for this msg example");

/* Main function of the Yielder process */
static int yielder(int argc, char* argv[])
{
  xbt_assert(argc == 2, "The sender function expects 1 arguments from the XML deployment file");
  long number_of_yields = xbt_str_parse_int(argv[1], "Invalid amount of yields: %s"); /* - number of yields */

  for (int i = 0; i < number_of_yields; i++)
    MSG_process_yield();
  XBT_INFO("I yielded %ld times. Goodbye now!", number_of_yields);
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  MSG_create_environment(argv[1]); /* - Load the platform description */

  MSG_function_register("yielder", yielder);
  MSG_launch_application(argv[2]); /* - Deploy the sender and receiver processes */

  msg_error_t res = MSG_main(); /* - Run the simulation */

  return res != MSG_OK;
}
