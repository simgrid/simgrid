/* Copyright (c) 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test_trace_integration, "Messages specific for this msg example");

/** test the trace integration cpu model */
static int test_trace(int argc, char *argv[])
{
  double task_comp_size = 2800;
  double task_prio = 1.0;

  xbt_assert (argc == 3,"Wrong number of arguments!\nUsage: %s <task computational size in FLOPS> <task priority>", argv[0]);

  task_comp_size = xbt_str_parse_double(argv[1],"Invalid computational size: %s");
  task_prio = xbt_str_parse_double(argv[2], "Invalid task priority: %s");

  XBT_INFO("Testing the trace integration cpu model: CpuTI");
  XBT_INFO("Task size: %f", task_comp_size);
  XBT_INFO("Task prio: %f", task_prio);

  /* Create and execute a single task. */
  msg_task_t task = MSG_task_create("proc 0", task_comp_size, 0, NULL);
  MSG_task_set_priority(task, task_prio);
  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_INFO("Test finished");

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s test_trace_integration_model.xml deployment.xml\n", argv[0]);

  MSG_function_register("test_trace", test_trace);
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  return res != MSG_OK;
}
