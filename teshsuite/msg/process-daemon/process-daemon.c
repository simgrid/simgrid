/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_process_daemon, "Messages specific for this msg example");

/* The worker process, working for a while before leaving */
static int worker_process(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Let's do some work (for 10 sec on Boivin).");
  msg_task_t task = MSG_task_create("easy work", 980.95e6, 0, NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_INFO("I'm done now. I leave even if it makes the daemon die.");
  return 0;
}

/* The daemon, displaying a message every 3 seconds until all other processes stop */
static int daemon_process(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  MSG_process_daemonize(MSG_process_self());

  while (1) {
    XBT_INFO("Hello from the infinite loop");
    MSG_process_sleep(3.0);
  }

  XBT_INFO("I will never reach that point: daemons are killed when regular processes are done");
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  MSG_process_create("worker", worker_process, NULL, xbt_dynar_getfirst_as(hosts, msg_host_t));
  MSG_process_create("daemon", daemon_process, NULL, xbt_dynar_getlast_as(hosts, msg_host_t));
  xbt_dynar_free(&hosts);
  msg_error_t res = MSG_main();

  return res != MSG_OK;
}
