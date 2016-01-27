/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"
#include <float.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this example");


static int launcher_fun(int argc, char *argv[])
{
  XBT_INFO("Launching");
  
  int host_nb = 2;

  msg_host_t* host_list = calloc(host_nb, sizeof(msg_host_t));
  host_list[0] = MSG_host_by_name("Jupiter");
  host_list[1] = MSG_host_by_name("Fafard");

  double* computation_amount = calloc(host_nb, sizeof(double));
  double* communication_amount = calloc(host_nb*host_nb, sizeof(double));

  int i,j;
  msg_error_t er;
  for(i=0;i<host_nb;i++) {
    computation_amount[i] = 100e6;
    for(j=0;j<host_nb;j++) {
      communication_amount[j + i*host_nb] = 100e6;
    }
  }
  {
    XBT_INFO("Starting a parallel task");
    msg_task_t ptask = MSG_parallel_task_create("taskname", host_nb, host_list, computation_amount, communication_amount, NULL);

    er = MSG_task_execute(ptask);
    if(er != MSG_OK)
      XBT_INFO("ERROR: %i", er);

    XBT_INFO("Done");
  }
  {
    XBT_INFO("Starting a parallel task with no communication");
    //MSG_task_execute free() computation_amount and communication_amount
    computation_amount = calloc(host_nb, sizeof(double));
    for(i=0;i<host_nb;i++) {
      computation_amount[i] = 100e6;
    }
    msg_task_t ptask = MSG_parallel_task_create("taskname", host_nb, host_list, computation_amount, NULL, NULL);

    er = MSG_task_execute( ptask);
    if(er != MSG_OK)
      XBT_INFO("ERROR: %i", er);

    XBT_INFO("Done");
  }
  
  XBT_INFO("Exiting");
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  /*   Application deployment */
  MSG_function_register("launcher", &launcher_fun);

  MSG_config("host/model", "ptask_L07");
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);
  res = MSG_main();

  return res != MSG_OK;
}
