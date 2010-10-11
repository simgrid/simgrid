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

/** The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(int argc, char *argv[])
{
  m_task_t task;
  INFO0
      ("I'll look for a new job on another machine where the grass is greener.");
  MSG_process_change_host(MSG_get_host_by_name("Boivin"));
  INFO0("Yeah, found something to do");
  task = MSG_task_create("job", 98095000, 0, NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  MSG_process_sleep(2);
  INFO0("Moving back home after work");
  MSG_process_change_host(MSG_get_host_by_name("Jacquelin"));
  MSG_process_change_host(MSG_get_host_by_name("Boivin"));
  MSG_process_sleep(4);
  INFO0("Uh, nothing to do here. Stopping now");
  return 0;
}                               /* end_of_emigrant */

/* This function would move the emigrant back home, if it were possible to do so in the MSG API.
 * Nothing for now.
 */
static int policeman(int argc, char *argv[])
{
  INFO0
      ("No function in the API to move the emigrant back, so do nothing.");
  return 0;
}                               /* end_of_policeman */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  /* Argument checking */
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    CRITICAL1("Usage: %s platform_file deployment_file\n", argv[0]);
    CRITICAL1("example: %s msg_platform.xml msg_deployment_suspend.xml\n",
              argv[0]);
    exit(1);
  }

  /* Simulation setting */
  MSG_create_environment(argv[1]);

  /* Application deployment */
  MSG_function_register("emigrant", emigrant);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  res = MSG_main();
  INFO1("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    res = MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
