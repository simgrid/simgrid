/* Copyright (c) 2010 The SimGrid team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/procmig.c</b> example to trace process migration using the mask TRACE_PROCESS
 */

#include "msg/msg.h"            /* core library */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(int argc, char *argv[])
{
  m_task_t task = NULL;
  char *destination = NULL;

  MSG_process_sleep(2);

  while (1){ // I am an eternal emigrant
    MSG_task_receive(&(task), "master_mailbox");
    destination = (char*)MSG_task_get_data (task);
    MSG_task_destroy (task);
    if (!destination) break; //there is no destination, die
    MSG_process_migrate(MSG_process_self(), MSG_get_host_by_name(destination));
    MSG_process_sleep(2); // I am tired, have to sleep for 2 seconds
    free (destination);
    task = NULL;
  }
  return 0;
}

static int master(int argc, char *argv[])
{
  m_task_t task = NULL;

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
    TRACE_msg_set_task_category(task, "migration_order");
    MSG_task_send (task, "master_mailbox");
    task = NULL;
  }
  xbt_dynar_free (&destinations);
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  /* Argument checking */
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  char *platform_file = argv[1];
  char *deployment_file = argv[2];
  MSG_create_environment(platform_file);

  TRACE_category ("migration_order");

  /* Application deployment */
  MSG_function_register("emigrant", emigrant);
  MSG_function_register("master", master);
  MSG_launch_application(deployment_file);

  MSG_main();
  MSG_clean();
  return 0;
}                               /* end_of_main */
