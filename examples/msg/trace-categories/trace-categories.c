/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/categories.c</b> This is a master/worker program. The master creates tasks and send them to workers.
 * For each task received, the worker executes it and then destroys it. This program declares several tracing categories
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

#include "simgrid/msg.h"

static int master(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  long workers_count = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");

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

  for (i = 0; i < workers_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 1000, 0);
    MSG_task_set_category(finalize, "finalize");
    MSG_task_send(finalize, "master_mailbox");
  }

  return 0;
}

static int worker(int argc, char *argv[])
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
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  //declaring user categories with RGB colors
  TRACE_category_with_color ("compute", "1 0 0"); //red
  TRACE_category_with_color ("request", "0 1 0"); //green
  TRACE_category_with_color ("data", "0 0 1");    //blue
  TRACE_category_with_color ("finalize", "0 0 0");//black

  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  MSG_main();
  return 0;
}
