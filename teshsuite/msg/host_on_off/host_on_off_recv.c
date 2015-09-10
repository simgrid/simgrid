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

static const char* mailbox = "comm";

/** Emitter function  */
int master(int argc, char *argv[])
{
  xbt_ex_t e;
  TRY {
    msg_host_t jupiter = MSG_host_by_name("Jupiter");
    
    XBT_INFO("Master starting");
    MSG_process_sleep(0.5);

    msg_comm_t comm = NULL;
    if (1) {
      msg_task_t task = MSG_task_create("COMM", 0, 100000000, NULL);
      comm = MSG_task_isend(task, mailbox);
    }

    if(MSG_process_sleep(0.5)) {
      XBT_ERROR("Unexpected error while sleeping");
      return 1;
    }
    XBT_INFO("Turning off the slave host");
    MSG_host_off(jupiter);

    if (comm) {
      MSG_comm_wait(comm, -1);
      MSG_comm_destroy(comm);
    }
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
    XBT_INFO("Slave receiving");
    msg_task_t task = NULL;
    msg_error_t error = MSG_task_receive(&(task), mailbox);
    if (error) {
      XBT_ERROR("Error while receiving message");
      return 1;
    }

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
  if (argc != 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
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

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
