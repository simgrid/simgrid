/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/forward.h"
#include "simgrid/mailbox.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#define FINALIZE 221297 /* a magic number to tell people to stop working */

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(app_masterworker, "Messages specific for this example");

/* Main function of the master process */
static void master(int argc, char* argv[])
{
  xbt_assert(argc == 5, "The master function expects 4 arguments from the XML deployment file");
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");       /* - Number of tasks      */
  double comp_size     = xbt_str_parse_double(argv[2], "Invalid computational size: %s"); /* - Compute cost    */
  long comm_size       = xbt_str_parse_int(argv[3], "Invalid communication size: %s");    /* - Communication size */
  long workers_count   = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");     /* - Number of workers    */

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (int i = 0; i < number_of_tasks; i++) { /* For each task to be executed: */
    char mailbox_name[80];
    char task_name[80];
    double* payload = xbt_malloc(sizeof(double));
    snprintf(mailbox_name, 79, "worker-%ld", i % workers_count); /* - Select a @ref worker in a round-robin way */
    snprintf(task_name, 79, "Task_%d", i);

    sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);
    *payload             = comp_size;

    if (number_of_tasks < 10000 || i % 10000 == 0)
      XBT_INFO("Sending \"%s\" (of %ld) to mailbox \"%s\"", task_name, number_of_tasks, mailbox_name);

    sg_mailbox_put(mailbox, payload, comm_size); /* - Send the amount of flops to compute to the @ref worker */
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) { /* - Eventually tell all the workers to stop by sending a "finalize" task */
    char mailbox_name[80];
    snprintf(mailbox_name, 79, "worker-%ld", i % workers_count);
    double* payload      = xbt_malloc(sizeof(double));
    sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);
    *payload             = FINALIZE;
    sg_mailbox_put(mailbox, payload, 0);
  }
}

/* Main functions of the Worker processes */
static void worker(int argc, char* argv[])
{
  xbt_assert(argc == 2,
             "The worker expects a single argument from the XML deployment file: its worker ID (its numerical rank)");
  char mailbox_name[80];

  long id = xbt_str_parse_int(argv[1], "Invalid argument %s");

  snprintf(mailbox_name, 79, "worker-%ld", id);
  sg_mailbox_t mailbox = sg_mailbox_by_name(mailbox_name);

  while (1) { /* The worker wait in an infinite loop for tasks sent by the @ref master */
    double* payload = (double*)sg_mailbox_get(mailbox);

    if (*payload == FINALIZE) {
      free(payload); /* - Exit if 'finalize' is received */
      break;
    }
    sg_actor_execute(*payload); /*  - Otherwise, process the received number of flops*/
    free(payload);
  }
  XBT_INFO("I'm done. See you!");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */

  simgrid_register_function("master", master); /* - Register the function to be executed by the processes */
  simgrid_register_function("worker", worker);
  simgrid_load_deployment(argv[2]); /* - Deploy the application */

  simgrid_run(); /* - Run the simulation */

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
