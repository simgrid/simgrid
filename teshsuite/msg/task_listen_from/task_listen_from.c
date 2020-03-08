/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is the test case of GitHub's #121 */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int tester(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_task_t task       = MSG_task_create("name", 0, 10, NULL);
  const_msg_comm_t comm = MSG_task_isend(task, "mailbox");

  XBT_INFO("MSG_task_listen_from returns() %d (should return my pid, which is %d)", MSG_task_listen_from("mailbox"),
           MSG_process_get_PID(MSG_process_self()));
  XBT_INFO("MSG_task_listen returns()      %d (should return true, i.e. 1)", MSG_task_listen("mailbox"));

  MSG_comm_destroy(comm);
  MSG_task_destroy(task);

  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);
  MSG_create_environment(argv[1]);

  MSG_process_create("tester", tester, NULL, MSG_get_host_by_name("Tremblay"));

  return MSG_main();
}
