/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int test(int argc, char* argv[])
{
  xbt_assert(argc == 3);
  double computation_amount = xbt_str_parse_double(argv[1], "Invalid argument: %s");
  double priority           = xbt_str_parse_double(argv[2], "Invalid argument: %s");

  XBT_INFO("Hello! Running a task of size %g with priority %g", computation_amount, priority);
  msg_task_t task = MSG_task_create("Task", computation_amount, 0.0, NULL);
  MSG_task_set_priority(task, priority);

  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_INFO("Goodbye now!");
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("test", test);
  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
