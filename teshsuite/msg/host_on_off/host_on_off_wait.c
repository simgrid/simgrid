/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */
#include "xbt/ex.h"

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
  xbt_ex_t e;
  TRY {
    msg_host_t jupiter = MSG_host_by_name("Jupiter");
    XBT_INFO("Master waiting");
    MSG_process_sleep(1);

    XBT_INFO("Turning off the slave host");
    MSG_host_off(jupiter);
    XBT_INFO("Master has finished");
  }
  CATCH(e) {
    xbt_die("Exception caught in the master");
    return 1;
  }
  return 0;
}

/** Receiver function  */
int slave(int argc, char *argv[])
{
  xbt_ex_t e;
  TRY {
    XBT_INFO("Slave waiting");
    // TODO, This should really be MSG_HOST_FAILURE
    MSG_process_sleep(5);
    XBT_ERROR("Slave should be off already.");
    return 1;
  }
  CATCH(e) {
    XBT_ERROR("Exception caught in the slave");
    return 1;
  }
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res;
  const char *platform_file;
  const char *application_file;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
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
