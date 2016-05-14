/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_app_masterworker, "Messages specific for this msg example");


/* Main function of the master process. It expects 4 arguments given in the XML deployment file: */
static int master(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");    /* - Number of tasks      */
  double comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");  /* - Task compute cost    */
  double comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");  /* - Task communication size */
  long workers_count = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");    /* - Number of workers    */

  int i;

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {  /* For each task to be executed: */
    char mailbox[256];
    char task_name[256];

    sprintf(mailbox, "worker-%ld", i % workers_count); /* - Select a @ref worker in a round-robin way */
    sprintf(task_name, "Task_%d", i);
    msg_task_t task = MSG_task_create(task_name, comp_size, comm_size, NULL);   /* - Create a task */
    if (number_of_tasks < 10000 || i % 10000 == 0)
      XBT_INFO("Sending \"%s\" (of %ld) to mailbox \"%s\"", task->name, number_of_tasks, mailbox);

    MSG_task_send(task, mailbox); /* - Send the task to the @ref worker */
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < workers_count; i++) { /* - Eventually tell all the workers to stop by sending a "finalize" task */
    char mailbox[80];

    sprintf(mailbox, "worker-%ld", i % workers_count);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox);
  }

  return 0;
}

/* Main functions of the Worker processes. It expects a single argument given in the XML deployment file: the unique ID of the worker. */
static int worker(int argc, char *argv[])
{
  msg_task_t task = NULL;
  char mailbox[80];

  long id= xbt_str_parse_int(argv[1], "Invalid argument %s");

  sprintf(mailbox, "worker-%ld", id);

  while (1) {  /* The worker wait in an infinite loop for tasks sent by the \ref master */
    int res = MSG_task_receive(&(task), mailbox);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

//  XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);  /* - Exit if 'finalize' is received */
      break;
    }
//    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);    /*  - Otherwise, process the task */
//    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);          /* - Load the platform description */

  MSG_function_register("master", master);  /* - Register the function to be executed by the processes */
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);          /* - Deploy the application */

  msg_error_t res = MSG_main();             /* - Run the simulation */

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
