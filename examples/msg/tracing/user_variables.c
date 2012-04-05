/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/user_variables.c</b>: This program demonstrates how to
 * trace user variables associated to the hosts of the platform file.
 * You might want to run this program with the following parameters:
 * --cfg=tracing:1
 * --cfg=tracing/platform:1
 * (See \ref tracing_tracing_options for details)
 */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

int master(int argc, char *argv[]);

int master(int argc, char *argv[])
{
  char *hostname = MSG_host_self()->name;
  int i;

  //the hostname has an empty HDD with a capacity of 100000 (bytes)
  TRACE_host_variable_set(hostname, "HDD_capacity", 100000);
  TRACE_host_variable_set(hostname, "HDD_utilization", 0);

  for (i = 0; i < 10; i++) {
    //create and execute a task just to make the simulated time advance
    m_task_t task = MSG_task_create("task", 10000, 0, NULL);
    MSG_task_execute (task);
    MSG_task_destroy (task);

    //ADD: after the execution of this task, the HDD utilization increases by 100 (bytes)
    TRACE_host_variable_add(hostname, "HDD_utilization", 100);
  }

  for (i = 0; i < 10; i++) {
    //create and execute a task just to make the simulated time advance
    m_task_t task = MSG_task_create("task", 10000, 0, NULL);
    MSG_task_execute (task);
    MSG_task_destroy (task);

    //SUB: after the execution of this task, the HDD utilization decreases by 100 (bytes)
    TRACE_host_variable_sub(hostname, "HDD_utilization", 100);
  }
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  char *platform_file = argv[1];
  char *deployment_file = argv[2];
  MSG_create_environment(platform_file);

  //declaring user variables
  TRACE_host_variable_declare("HDD_capacity");
  TRACE_host_variable_declare("HDD_utilization");

  //register functions and launch deployment
  MSG_function_register("master", master);
  MSG_function_register("slave", master);
  MSG_launch_application(deployment_file);

  MSG_main();

  //get user declared variables
  unsigned int cursor;
  char *variable;
  xbt_dynar_t host_variables = TRACE_get_host_variables ();
  if (host_variables){
    XBT_INFO ("Declared host variables:");
    xbt_dynar_foreach (host_variables, cursor, variable){
      XBT_INFO ("%s", variable);
    }
    xbt_dynar_free (&host_variables);
  }
  xbt_dynar_t link_variables = TRACE_get_link_variables ();
  if (link_variables){
    XBT_INFO ("Declared link variables:");
    xbt_dynar_foreach (link_variables, cursor, variable){
      XBT_INFO ("%s", variable);
    }
    xbt_dynar_free (&link_variables);
  }

  MSG_clean();
  return 0;
}
