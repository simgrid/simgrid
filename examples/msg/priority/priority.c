/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>priority/priority.c</b>: Demonstrates the use of @ref
 *   MSG_task_set_priority to change the computation priority of a  given task.
 */

static int test(int argc, char *argv[])
{
  double computation_amount = 0.0;
  double priority = 1.0;
  msg_task_t task = NULL;

  XBT_ATTRIB_UNUSED int res = sscanf(argv[1], "%lg", &computation_amount);
  xbt_assert(res, "Invalid argument %s\n", argv[1]);
  res = sscanf(argv[2], "%lg", &priority);
  xbt_assert(res, "Invalid argument %s\n", argv[2]);

  XBT_INFO("Hello! Running a task of size %g with priority %g", computation_amount, priority);
  task = MSG_task_create("Task", computation_amount, 0.0, NULL);
  MSG_task_set_priority(task, priority);

  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_INFO("Goodbye now!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("test", test);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
