/* Copyright (c) 2007, 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 *
 *  - <b>Life-Cycle: process-startkilltime/process-startkilltime.c</b>. The <i>creation</i> and  <i>termination</i>
 *    times of processes can be explicitly given in the deployment file (see *_d.xml files in example directory).
 */

/** Executed on process termination*/
static int my_onexit(void* ignored1, void *ignored2) {
  XBT_INFO("Exiting now (done sleeping or got killed)."); /** - Just display an informative message (see tesh file) */
  return 0;
}

/** Just sleep until termination */
static int sleeper(int argc, char *argv[])
{
  XBT_INFO("Hello! I go to sleep.");
  MSG_process_on_exit(my_onexit, NULL);

  MSG_process_sleep(xbt_str_parse_int(argv[1], "sleeper process expects an integer parameter but got %s"));
  XBT_INFO("Done sleeping.");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);   /** - Load the platform description */
  MSG_function_register("sleeper", sleeper);
  MSG_launch_application(argv[2]);   /** - Deploy the @ref sleeper processes with explicit start/kill times */

  res = MSG_main();                  /** - Run the simulation */
  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
