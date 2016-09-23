/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include <float.h>
XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific to this example");

static int sender_fun(int argc, char *argv[])
{
  XBT_INFO("Sending");
  MSG_task_send(MSG_task_create("Blah", 0.0, 0.0, NULL), MSG_host_get_name(MSG_host_self()));
  MSG_process_sleep(1.);     /* FIXME: if the sender exits before the receiver calls get_sender(), bad thing happens */
  XBT_INFO("Exiting");
  return 0;
}

static int receiver_fun(int argc, char *argv[])
{
  XBT_INFO("Receiving");
  msg_task_t task = NULL;
  MSG_task_receive_with_timeout(&task, MSG_host_get_name(MSG_host_self()), DBL_MAX);
  xbt_assert(MSG_task_get_sender(task), "No sender received");
  XBT_INFO("Got a message sent by '%s'", MSG_process_get_name(MSG_task_get_sender(task)));
  MSG_task_destroy(task);
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  MSG_create_environment(argv[1]);

  MSG_process_create("send", sender_fun, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("receive", receiver_fun, NULL, MSG_get_host_by_name("Tremblay"));

  res = MSG_main();
  return res != MSG_OK;
}
