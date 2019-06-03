/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/msg.h>
#include <simgrid/modelchecker.h>
#define N 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");

static int server(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_task_t task1 = NULL;
  msg_task_t task2 = NULL;

  MSG_task_receive(&task1, "mymailbox");
  long val1 = xbt_str_parse_int(MSG_task_get_name(task1), "Task name is not a numerical ID: %s");
  MSG_task_destroy(task1);
  task1 = NULL;
  XBT_INFO("Received %ld", val1);

  MSG_task_receive(&task2, "mymailbox");
  long val2 = xbt_str_parse_int(MSG_task_get_name(task2), "Task name is not a numerical ID: %s");
  MSG_task_destroy(task2);
  task2 = NULL;
  XBT_INFO("Received %ld", val2);

  MC_assert(MIN(val1, val2) == 1);

  MSG_task_receive(&task1, "mymailbox");
  val1 = xbt_str_parse_int(MSG_task_get_name(task1), "Task name is not a numerical ID: %s");
  MSG_task_destroy(task1);
  XBT_INFO("Received %ld", val1);

  MSG_task_receive(&task2, "mymailbox");
  val2 = xbt_str_parse_int(MSG_task_get_name(task2), "Task name is not a numerical ID: %s");
  MSG_task_destroy(task2);
  XBT_INFO("Received %ld", val2);

  XBT_INFO("OK");
  return 0;
}

static int client(int argc, char *argv[])
{
  xbt_assert(argc == 2);
  msg_task_t task1 = MSG_task_create(argv[1], 0, 10000, NULL);
  msg_task_t task2 = MSG_task_create(argv[1], 0, 10000, NULL);

  XBT_INFO("Send %s", argv[1]);
  MSG_task_send(task1, "mymailbox");

  XBT_INFO("Send %s", argv[1]);
  MSG_task_send(task2, "mymailbox");

  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);
  MSG_function_register("client", client);
  MSG_launch_application("deploy_bugged2.xml");

  MSG_main();
  return 0;
}
