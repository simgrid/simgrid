/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** Master expects 3+ arguments given in the XML deployment file: */
static int master(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");    /** - Number of tasks      */
  double comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");  /** - Task compute cost    */
  double comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");  /** - Task communication size */

  /* Create the tasks in advance */
  msg_task_t* todo = xbt_new0(msg_task_t, number_of_tasks);

  for (int i = 0; i < number_of_tasks; i++) {
    char sprintf_buffer[64];
    sprintf(sprintf_buffer, "Task_%d", i);
    todo[i] = MSG_task_create(sprintf_buffer, comp_size, comm_size, NULL);
  }

  /* Get the info about the worker processes from my parameters */
  int workers_count   = argc - 4;
  msg_host_t* workers = xbt_new0(msg_host_t, workers_count);

  for (int i = 4; i < argc; i++) {
    workers[i - 4] = MSG_get_host_by_name(argv[i]);
    xbt_assert(workers[i - 4] != NULL, "Unknown host %s. Stopping Now! ", argv[i]);
  }
  XBT_INFO("Got %d workers and %ld tasks to process", workers_count, number_of_tasks);

  /* Dispatch the tasks */
  for (int i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending '%s' to '%s'", todo[i]->name, MSG_host_get_name(workers[i % workers_count]));
    if (MSG_host_self() == workers[i % workers_count]) {
      XBT_INFO("Hey ! It's me ! :)");
    }

    MSG_task_send(todo[i], MSG_host_get_name(workers[i % workers_count]));
    XBT_INFO("Sent");
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, MSG_host_get_name(workers[i]));
  }

  XBT_INFO("Goodbye now!");
  free(workers);
  free(todo);
  return 0;
}

/** Worker expects a single argument given in the XML deployment file: */
static int worker(int argc, char *argv[])
{
  while (1) {
    msg_task_t task = NULL;
    int res         = MSG_task_receive(&(task), MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");

    XBT_INFO("Received '%s'", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("'%s' done", MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_worker */

/** Main function */
int main(int argc, char *argv[])
{

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  /*  Create a simulated platform */
  MSG_create_environment(argv[1]);

  /*   Application deployment */
  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return (res != MSG_OK);
}                               /* end_of_main */
