/* Copyright (c) 2013-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* This example implements one process which receives messages from two other */
/* processes. There is no bug on it, it is just provided to test the soundness*/
/* of the state space reduction with DPOR, if the maximum depth (defined with */
/* --cfg=model-check/max-depth:) is reached.                                  */
/******************************************************************************/

#include <simgrid/msg.h>
#include <simgrid/modelchecker.h>

#define N 2

XBT_LOG_NEW_DEFAULT_CATEGORY(electric_fence, "Example to check the soundness of DPOR");

static int server(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_task_t task1 = NULL;
  msg_task_t task2 = NULL;

  msg_comm_t comm_received1 = MSG_task_irecv(&task1, "mymailbox");
  msg_comm_t comm_received2 = MSG_task_irecv(&task2, "mymailbox");

  MSG_comm_wait(comm_received1, -1);
  MSG_comm_wait(comm_received2, -1);

  XBT_INFO("OK");
  return 0;
}

static int client(int argc, char *argv[])
{
  xbt_assert(argc == 2);
  msg_task_t task = MSG_task_create(argv[1], 0, 10000, NULL);

  MSG_task_send(task, "mymailbox");

  XBT_INFO("Sent!");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_electric_fence.xml");

  MSG_main();
  return 0;
}
