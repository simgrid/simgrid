/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  - <b>masterworker/masterworker.c: Master/workers example</b>. This good old example is also very simple. Its
 *    basic version is fully commented on this page: \ref MSG_ex_master_worker, but several variants can be found in the
 *    same directory.
 */

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static int master(int argc, char *argv[])
{
  msg_host_t *workers = NULL;
  msg_task_t *todo = NULL;
  long workers_count = 0;
  int i;

  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(msg_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    }
  }

  {                             /* Process organization */
    workers_count = argc - 4;
    workers = xbt_new0(msg_host_t, workers_count);

    for (i = 4; i < argc; i++) {
      workers[i - 4] = MSG_host_by_name(argv[i]);
      xbt_assert(workers[i - 4] != NULL, "Unknown host %s. Stopping Now! ", argv[i]);
    }
  }

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);
  for (i = 0; i < workers_count; i++)
    XBT_DEBUG("%s", MSG_host_get_name(workers[i]));

  for (i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending \"%s\" to \"%s\"", todo[i]->name, MSG_host_get_name(workers[i % workers_count]));
    if (MSG_host_self() == workers[i % workers_count]) {
      XBT_INFO("Hey ! It's me ! :)");
    }

    MSG_task_send(todo[i], MSG_host_get_name(workers[i % workers_count]));
    XBT_INFO("Sent");
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < workers_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, MSG_host_get_name(workers[i]));
  }

  XBT_INFO("Goodbye now!");
  free(workers);
  free(todo);
  return 0;
}

static int worker(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED int res;
  while (1) {
    res = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
