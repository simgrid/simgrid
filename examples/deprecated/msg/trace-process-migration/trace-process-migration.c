/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

/* The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_task_t task = NULL;
  char *destination = NULL;

  MSG_process_sleep(2);

  while (1){ // I am an eternal emigrant
    MSG_task_receive(&(task), "master_mailbox");
    destination = (char*)MSG_task_get_data (task);
    MSG_task_destroy (task);
    if (destination == NULL)
      break; //there is no destination, die
    MSG_process_migrate(MSG_process_self(), MSG_host_by_name(destination));
    MSG_process_sleep(2); // I am tired, have to sleep for 2 seconds
    free (destination);
    task = NULL;
  }
  return 0;
}

static int policeman(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
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
    msg_task_t task = MSG_task_create("task", 0, 0, NULL);
    if (destination != NULL){
      MSG_task_set_data(task, xbt_strdup (destination));
    }
    MSG_task_set_category(task, "migration_order");
    MSG_task_send (task, "master_mailbox");
  }
  xbt_dynar_free (&destinations);
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  TRACE_category ("migration_order");

  MSG_process_create("emigrant", emigrant, NULL, MSG_get_host_by_name("Fafard"));
  MSG_process_create("policeman", policeman, NULL, MSG_get_host_by_name("Tremblay"));

  MSG_main();
  return 0;
}
