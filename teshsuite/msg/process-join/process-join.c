/* Copyright (c) 2010-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int slave(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  XBT_INFO("Slave started");
  MSG_process_sleep(3);
  XBT_INFO("I'm done. See you!");
  return 0;
}

static int master(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_process_t process;

  XBT_INFO("Start slave");
  process = MSG_process_create("slave from master", slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 2)");
  MSG_process_join(process, 2);

  XBT_INFO("Start slave");
  process = MSG_process_create("slave from master", slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 4)");
  MSG_process_join(process, 4);

  XBT_INFO("Start slave");
  process = MSG_process_create("slave from master", slave, NULL, MSG_host_self());
  XBT_INFO("Join the slave (timeout 2)");
  MSG_process_join(process, 2);

  XBT_INFO("Start slave");
  process = MSG_process_create("slave from master", slave, NULL, MSG_host_self());
  MSG_process_ref(process); // We have to take that ref because the process will stop before we join it
  XBT_INFO("Waiting 4");
  MSG_process_sleep(4);
  XBT_INFO("Join the slave after its end (timeout 1)");
  MSG_process_join(process, 1);
  MSG_process_unref(process); // Avoid to leak memory

  XBT_INFO("Goodbye now!");

  MSG_process_sleep(1);

  XBT_INFO("Goodbye now!");
  return 0;
}

int main(int argc, char* argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("master", master, NULL, MSG_get_host_by_name("Tremblay"));

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
