/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt/synchro.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_process_migration, "Messages specific for this msg example");

xbt_mutex_t checkpoint = NULL;
xbt_cond_t identification = NULL;
static msg_process_t controlled_process = NULL;

/* The Emigrant process will be moved from host to host. */
static int emigrant(int argc, char *argv[])
{
  XBT_INFO("I'll look for a new job on another machine ('Boivin') where the grass is greener.");
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Boivin"));    /* - First, move to another host by myself */

  XBT_INFO("Yeah, found something to do");
  msg_task_t task = MSG_task_create("job", 98095000, 0, NULL);            /* - Execute some work there */
  MSG_task_execute(task);
  MSG_task_destroy(task);
  MSG_process_sleep(2);
  XBT_INFO("Moving back home after work");
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Jacquelin")); /* - Move back to original location */
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Boivin"));    /* - Go back to the other host to sleep*/
  MSG_process_sleep(4);
  xbt_mutex_acquire(checkpoint);                                          /* - Get controlled at checkpoint */
  controlled_process = MSG_process_self();                                /* - and get moved back by the policeman process */
  xbt_cond_broadcast(identification);
  xbt_mutex_release(checkpoint);
  MSG_process_suspend(MSG_process_self());
  msg_host_t h = MSG_process_get_host(MSG_process_self());
  XBT_INFO("I've been moved on this new host: %s", MSG_host_get_name(h));
  XBT_INFO("Uh, nothing to do here. Stopping now");
  return 0;
}

/* The policeman check for emigrants and move them back to 'Jacquelin' */
static int policeman(int argc, char *argv[])
{
  xbt_mutex_acquire(checkpoint);
  XBT_INFO("Wait at the checkpoint.");  /* - block on the mutex+condition */
  while (controlled_process == NULL) xbt_cond_wait(identification, checkpoint);
  MSG_process_migrate(controlled_process, MSG_host_by_name("Jacquelin")); /* - Move an emigrant to Jacquelin */
  XBT_INFO("I moved the emigrant");
  MSG_process_resume(controlled_process);
  xbt_mutex_release(checkpoint);

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);  /* - Load the platform description */
  /* - Create and deploy the emigrant and policeman processes */
  MSG_process_create("emigrant", emigrant, NULL, MSG_get_host_by_name("Jacquelin"));
  MSG_process_create("policeman", policeman, NULL, MSG_get_host_by_name("Boivin"));

  checkpoint      = xbt_mutex_init(); /* - Initiate the mutex and conditions */
  identification = xbt_cond_init();
  msg_error_t res = MSG_main(); /* - Run the simulation */
  XBT_INFO("Simulation time %g", MSG_get_clock());
  xbt_cond_destroy(identification);
  xbt_mutex_destroy(checkpoint);

  return res != MSG_OK;
}
