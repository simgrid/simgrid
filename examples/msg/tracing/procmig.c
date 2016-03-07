/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/procmig.c</b> This program shows a process migration. Tracing this program with the options below
 * enables a gantt-chart visualization of where the process has been during its execution. Migrations are represented by
 * arrows from the origin to the destination host. You might want to run this program with the following parameters:
 * --cfg=tracing:yes
 * --cfg=tracing/msg/process:yes
 * (See \ref tracing_tracing_options for details)
 */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(int argc, char *argv[])
{
  msg_task_t task = NULL;
  char *destination = NULL;

  MSG_process_sleep(2);

  while (1){ // I am an eternal emigrant
    MSG_task_receive(&(task), "master_mailbox");
    destination = (char*)MSG_task_get_data (task);
    MSG_task_destroy (task);
    if (!destination) break; //there is no destination, die
    MSG_process_migrate(MSG_process_self(), MSG_host_by_name(destination));
    MSG_process_sleep(2); // I am tired, have to sleep for 2 seconds
    free (destination);
    task = NULL;
  }
  return 0;
}

static int master(int argc, char *argv[])
{
  msg_task_t task = NULL;

  // I am the master of emigrant process,
  // I tell it where it must emigrate to.
  xbt_dynar_t destinations = xbt_dynar_new (sizeof(char*), &xbt_free_ref);
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Tremblay"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Jupiter"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Fafard"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Ginette"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Bourassa"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Fafard"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Tremblay"));
  xbt_dynar_push_as (destinations, char*, xbt_strdup ("Ginette"));
  xbt_dynar_push_as (destinations, char*, NULL);

  char *destination;
  unsigned int i;
  xbt_dynar_foreach(destinations, i, destination){
    task = MSG_task_create("task", 0, 0, NULL);
    if (destination){
      MSG_task_set_data(task, xbt_strdup (destination));
    }
    MSG_task_set_category(task, "migration_order");
    MSG_task_send (task, "master_mailbox");
    task = NULL;
  }
  xbt_dynar_free (&destinations);
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  if (argc < 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  MSG_create_environment(argv[1]);

  TRACE_category ("migration_order");

  /* Application deployment */
  MSG_function_register("emigrant", emigrant);
  MSG_function_register("master", master);
  MSG_launch_application(argv[2]);

  MSG_main();
  return 0;
}                               /* end_of_main */
