/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/**************** Shared buffer between asynchronous receives *****************/
/* Server process assumes that the data from the second communication comm2   */
/* will overwrite the one from the first communication, because of the order  */
/* of the wait calls. This is not true because data copy can be triggered by  */
/* a call to wait on the other end of the communication (client).             */
/* NOTE that the communications use different mailboxes, but they share the   */
/* same buffer for reception (task1).                                         */
/******************************************************************************/

#include <simgrid/msg.h>
#include <simgrid/modelchecker.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged3, "this example");


static int server(int argc, char *argv[])
{
  msg_task_t task1,task2;

  msg_comm_t comm1 = MSG_task_irecv(&task1, "mymailbox1");
  msg_comm_t comm2 = MSG_task_irecv(&task2, "mymailbox2");
  MSG_comm_wait(comm1, -1);
  MSG_comm_wait(comm2, -1);

  long val1 = xbt_str_parse_int(MSG_task_get_name(task1), "Task name is not a numerical ID: %s");
  XBT_INFO("Received %lu", val1);

  MC_assert(val1 == 2);

  XBT_INFO("OK");
  return 0;
}

static int client(int argc, char *argv[])
{
  msg_task_t task1 = MSG_task_create(argv[1], 0, 10000, NULL);

  char *mbox = bprintf("mymailbox%s", argv[1]);

  XBT_INFO("Send %s!", argv[1]);
  msg_comm_t comm = MSG_task_isend(task1, mbox);
  MSG_comm_wait(comm, -1);

  xbt_free(mbox);

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_bugged3.xml");

  MSG_main();
  return 0;
}
