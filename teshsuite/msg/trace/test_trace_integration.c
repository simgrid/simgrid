/* Copyright (c) 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test_trace_integration,
                             "Messages specific for this msg example");

int test_trace(int argc, char *argv[]);

/** test the trace integration cpu model */
int test_trace(int argc, char *argv[])
{
  msg_task_t task;
  double task_comp_size = 2800;
  double task_prio = 1.0;

  xbt_assert (argc == 3,"Wrong number of arguments!\nUsage: %s <task computational size in FLOPS> <task priority>", argv[0]);

  task_comp_size = xbt_str_parse_double(argv[1],"Invalid computational size: %s");
  task_prio = xbt_str_parse_double(argv[2], "Invalid task priority: %s");

  XBT_INFO("Testing the trace integration cpu model: CpuTI");
  XBT_INFO("Task size: %f", task_comp_size);
  XBT_INFO("Task prio: %f", task_prio);

  /* Create and execute a single task. */
  task = MSG_task_create("proc 0", task_comp_size, 0, NULL);
  MSG_task_set_priority(task, task_prio);
  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_INFO("Test finished");

  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  /* Verify if the platform xml file was passed by command line. */
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s test_trace_integration_model.xml deployment.xml\n", argv[0]);

  /* Register SimGrid process function. */
  MSG_function_register("test_trace", test_trace);
  /* Use the same file for platform and deployment. */
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);
  /* Run the example. */
  res = MSG_main();

  return res != MSG_OK;
}
