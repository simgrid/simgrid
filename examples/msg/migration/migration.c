/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"            /* core library */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 *  
 *  - <b>migration/migration.c</b> Demonstrates how to use the @ref
 *    MSG_process_migrate function to let processes change the host they 
 *    run on after their start. 
 */

xbt_mutex_t mutex = NULL;
xbt_cond_t cond = NULL;
static m_process_t process_to_migrate = NULL;

/** The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(int argc, char *argv[])
{
  m_task_t task;
  XBT_INFO
      ("I'll look for a new job on another machine where the grass is greener.");
  MSG_process_migrate(MSG_process_self(), MSG_get_host_by_name("Boivin"));
  
  XBT_INFO("Yeah, found something to do");
  task = MSG_task_create("job", 98095000, 0, NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  MSG_process_sleep(2);
  XBT_INFO("Moving back home after work");
  MSG_process_migrate(MSG_process_self(), MSG_get_host_by_name("Jacquelin"));
  MSG_process_migrate(MSG_process_self(), MSG_get_host_by_name("Boivin"));
  MSG_process_sleep(4);
  xbt_mutex_acquire(mutex);
  process_to_migrate = MSG_process_self();
  xbt_cond_broadcast(cond);
  xbt_mutex_release(mutex);
  MSG_process_suspend(MSG_process_self());
  m_host_t h = MSG_process_get_host(MSG_process_self());
  XBT_INFO("I've been moved on this new host: %s", h->name);
  XBT_INFO("Uh, nothing to do here. Stopping now");
  return 0;
}                               /* end_of_emigrant */


/* This function move the emigrant on Jacquelin */
static int policeman(int argc, char *argv[])
{

  xbt_mutex_acquire(mutex);
  XBT_INFO("Wait a bit before migrating the emigrant.");
  while (process_to_migrate == NULL) xbt_cond_wait(cond, mutex);
  MSG_process_migrate(process_to_migrate, MSG_get_host_by_name("Jacquelin"));
  XBT_INFO("I moved the emigrant");
  MSG_process_resume(process_to_migrate);
  xbt_mutex_release(mutex);

  return 0;
}                               /* end_of_policeman */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  /* Argument checking */
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    XBT_CRITICAL("example: %s msg_platform.xml msg_deployment_suspend.xml\n",
              argv[0]);
    exit(1);
  }

  /* Simulation setting */
  MSG_create_environment(argv[1]);

  /* Application deployment */
  MSG_function_register("emigrant", emigrant);
  MSG_function_register("policeman", policeman);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  mutex = xbt_mutex_init();
  cond = xbt_cond_init();
  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  xbt_cond_destroy(cond);
  xbt_mutex_destroy(mutex);

  if (res == MSG_OK)
    res = MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
