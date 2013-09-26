/* Copyright (c) 2009-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include <float.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this example");


static int send(int argc, char *argv[])
{
  XBT_INFO("Sending");
  MSG_task_send(MSG_task_create("Blah", 0.0, 0.0, NULL), MSG_host_get_name(MSG_host_self()));
  MSG_process_sleep(1.);        /* FIXME: if the sender exits before the receiver calls get_sender(), bad thing happens */
  XBT_INFO("Exiting");
  return 0;
}

static int receive(int argc, char *argv[])
{
  XBT_INFO("Receiving");
  msg_task_t task = NULL;
  MSG_task_receive_with_timeout(&task, MSG_host_get_name(MSG_host_self()), DBL_MAX);
  xbt_assert(MSG_task_get_sender(task), "No sender received");
  XBT_INFO("Got a message sent by '%s'",
        MSG_process_get_name(MSG_task_get_sender(task)));
  MSG_task_destroy(task);
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  /*   Application deployment */
  MSG_function_register("send", &send);
  MSG_function_register("receive", &receive);

  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);
  res = MSG_main();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
