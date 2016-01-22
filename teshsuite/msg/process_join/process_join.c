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
  msg_process_t process;

  XBT_INFO("Start slave");
  process =  MSG_process_create("slave from master",
                               slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 2)");
  MSG_process_join(process, 2);

  XBT_INFO("Start slave");
  process =  MSG_process_create("slave from master",
                               slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 4)");
  MSG_process_join(process, 4);

  XBT_INFO("Start slave");
  process =  MSG_process_create("slave from master",
                               slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 2)");
  MSG_process_join(process, 2);

  XBT_INFO("Goodbye now!");

  MSG_process_sleep(1);

  XBT_INFO("Goodbye now!");
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  XBT_INFO("Slave started");
  MSG_process_sleep(3);
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
    "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

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
