/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);

/** Emitter function  */
int master(int argc, char *argv[])
{
  xbt_swag_t process_list = MSG_host_get_process_list(MSG_host_self());
  msg_process_t process = NULL;
  MSG_process_sleep(1);
  xbt_swag_foreach(process, process_list) {
    XBT_INFO("Process(pid=%d, ppid=%d, name=%s)",
             MSG_process_get_PID(process), MSG_process_get_PPID(process),
             MSG_process_get_name(process));
    if (MSG_process_self_PID() != MSG_process_get_PID(process))
      MSG_process_kill(process);
  }
  process = MSG_process_create("slave from master",
                               slave, NULL, MSG_host_self());
  MSG_process_sleep(2);

  XBT_INFO("Suspend Process(pid=%d)", MSG_process_get_PID(process));
  MSG_process_suspend(process);

  XBT_INFO("Process(pid=%d) is %ssuspended",
           MSG_process_get_PID(process),
           (MSG_process_is_suspended(process)) ? "" : "not ");
  MSG_process_sleep(2);

  XBT_INFO("Resume Process(pid=%d)", MSG_process_get_PID(process));
  MSG_process_resume(process);

  XBT_INFO("Process(pid=%d) is %ssuspended",
           MSG_process_get_PID(process),
           (MSG_process_is_suspended(process)) ? "" : "not ");
  MSG_process_sleep(2);
  MSG_process_kill(process);

  XBT_INFO("Goodbye now!");
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  MSG_process_sleep(.5);
  XBT_INFO("Slave started (PID:%d, PPID:%d)",
           MSG_process_self_PID(), MSG_process_self_PPID());
  while(1){
    XBT_INFO("Plop i am %ssuspended",
             (MSG_process_is_suspended(MSG_process_self())) ? "" : "not ");
    MSG_process_sleep(1);
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res;
  const char *platform_file;
  const char *application_file;

  MSG_init(&argc, argv);
  xbt_assert(argc == 3, "Usage: %s platform_file deployment_file\n"
	     "\n Example: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);
 
  platform_file = argv[1];
  application_file = argv[2];

  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);

    MSG_launch_application(application_file);
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}                               /* end_of_main */
