/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/categories.c</b> This is a master/slave program where the master creates tasks, send them to the slaves.
 * For each task received, the slave executes it and then destroys it. This program declares several tracing categories
 * that are used to classify tasks. When the program is executed, the tracing mechanism registers the resource
 * utilization of hosts and links according to these categories. You might want to run this program with the following
 * parameters:
 * --cfg=tracing:yes
 * --cfg=tracing/categorized:yes
 * --cfg=tracing/uncategorized:yes
 * --cfg=viva/categorized:viva_cat.plist
 * --cfg=viva/uncategorized:viva_uncat.plist
 * (See \ref tracing_tracing_options for details)
 */

#include <stdio.h>
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int master(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  long slaves_count = xbt_str_parse_int(argv[4], "Invalid amount of slaves: %s");

  int i;
  for (i = 0; i < number_of_tasks; i++) {
    msg_task_t task = NULL;

    //creating task and setting its category
    if (i % 2) {
      task = MSG_task_create("task_compute", 10000000, 0, NULL);
      MSG_task_set_category(task, "compute");
    } else if (i % 3) {
      task = MSG_task_create("task_request", 10, 10, NULL);
      MSG_task_set_category(task, "request");
    } else {
      task = MSG_task_create("task_data", 10, 10000000, NULL);
      MSG_task_set_category(task, "data");
    }
    MSG_task_send(task, "master_mailbox");
  }

  for (i = 0; i < slaves_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 1000, 0);
    MSG_task_set_category(finalize, "finalize");
    MSG_task_send(finalize, "master_mailbox");
  }

  return 0;
}

static int slave(int argc, char *argv[])
{
  msg_task_t task = NULL;

  while (1) {
    MSG_task_receive(&(task), "master_mailbox");

    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    MSG_task_execute(task);
    MSG_task_destroy(task);
    task = NULL;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  MSG_create_environment(argv[1]);

  //declaring user categories with RGB colors
  TRACE_category_with_color ("compute", "1 0 0"); //red
  TRACE_category_with_color ("request", "0 1 0"); //green
  TRACE_category_with_color ("data", "0 0 1");    //blue
  TRACE_category_with_color ("finalize", "0 0 0");//black

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_launch_application(argv[2]);

  MSG_main();
  return 0;
}
