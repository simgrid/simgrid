/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int trace_fun(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char *hostname = MSG_host_get_name(MSG_host_self());

  //the hostname has an empty HDD with a capacity of 100000 (bytes)
  TRACE_host_variable_set(hostname, "HDD_capacity", 100000);
  TRACE_host_variable_set(hostname, "HDD_utilization", 0);

  for (int i = 0; i < 10; i++) {
    //create and execute a task just to make the simulated time advance
    msg_task_t task = MSG_task_create("task", 10000, 0, NULL);
    MSG_task_execute (task);
    MSG_task_destroy (task);

    //ADD: after the execution of this task, the HDD utilization increases by 100 (bytes)
    TRACE_host_variable_add(hostname, "HDD_utilization", 100);
  }

  for (int i = 0; i < 10; i++) {
    //create and execute a task just to make the simulated time advance
    msg_task_t task = MSG_task_create("task", 10000, 0, NULL);
    MSG_task_execute (task);
    MSG_task_destroy (task);

    //SUB: after the execution of this task, the HDD utilization decreases by 100 (bytes)
    TRACE_host_variable_sub(hostname, "HDD_utilization", 100);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  //declaring user variables
  TRACE_host_variable_declare("HDD_capacity");
  TRACE_host_variable_declare("HDD_utilization");

  MSG_process_create("master", trace_fun, NULL, MSG_host_by_name("Tremblay"));

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
  return 0;
}
