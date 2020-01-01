/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_process_migration, "Messages specific for this msg example");

msg_bar_t barrier;
static msg_process_t controlled_process = NULL;

/* The Emigrant process will be moved from host to host. */
static int emigrant(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("I'll look for a new job on another machine ('Boivin') where the grass is greener.");
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Boivin")); /* - First, move to another host by myself */

  XBT_INFO("Yeah, found something to do");
  msg_task_t task = MSG_task_create("job", 98095000, 0, NULL); /* - Execute some work there */
  MSG_task_execute(task);
  MSG_task_destroy(task);
  MSG_process_sleep(2);
  XBT_INFO("Moving back home after work");
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Jacquelin")); /* - Move back to original location */
  MSG_process_migrate(MSG_process_self(), MSG_host_by_name("Boivin"));    /* - Go back to the other host to sleep*/
  MSG_process_sleep(4);
  controlled_process = MSG_process_self(); /* - Get controlled at checkpoint */
  MSG_barrier_wait(barrier);
  MSG_process_suspend(MSG_process_self());
  const_sg_host_t h = MSG_process_get_host(MSG_process_self());
  XBT_INFO("I've been moved on this new host: %s", MSG_host_get_name(h));
  XBT_INFO("Uh, nothing to do here. Stopping now");
  return 0;
}

/* The policeman check for emigrants and move them back to 'Jacquelin' */
static int policeman(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Wait at the checkpoint."); /* - block on the mutex+condition */
  MSG_barrier_wait(barrier);
  MSG_process_migrate(controlled_process, MSG_host_by_name("Jacquelin")); /* - Move an emigrant to Jacquelin */
  XBT_INFO("I moved the emigrant");
  MSG_process_resume(controlled_process);

  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]); /* - Load the platform description */
  /* - Create and deploy the emigrant and policeman processes */
  MSG_process_create("emigrant", emigrant, NULL, MSG_get_host_by_name("Jacquelin"));
  MSG_process_create("policeman", policeman, NULL, MSG_get_host_by_name("Boivin"));

  barrier         = MSG_barrier_init(2);
  msg_error_t res = MSG_main(); /* - Run the simulation */
  XBT_INFO("Simulation time %g", MSG_get_clock());
  MSG_barrier_destroy(barrier);

  return res != MSG_OK;
}
