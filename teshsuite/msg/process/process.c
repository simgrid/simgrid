/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int slave(int argc, char *argv[])
{
  MSG_process_sleep(.5);
  XBT_INFO("Slave started (PID:%d, PPID:%d)", MSG_process_self_PID(), MSG_process_self_PPID());
  while(1){
    XBT_INFO("Plop i am %ssuspended", (MSG_process_is_suspended(MSG_process_self())) ? "" : "not ");
    MSG_process_sleep(1);
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

static int master(int argc, char *argv[])
{
  xbt_swag_t process_list = MSG_host_get_process_list(MSG_host_self());
  msg_process_t process = NULL;
  MSG_process_sleep(1);
  xbt_swag_foreach(process, process_list) {
    XBT_INFO("Process(pid=%d, ppid=%d, name=%s)", MSG_process_get_PID(process), MSG_process_get_PPID(process),
             MSG_process_get_name(process));
    if (MSG_process_self_PID() != MSG_process_get_PID(process))
      MSG_process_kill(process);
  }
  process = MSG_process_create("slave from master", slave, NULL, MSG_host_self());
  MSG_process_sleep(2);

  XBT_INFO("Suspend Process(pid=%d)", MSG_process_get_PID(process));
  MSG_process_suspend(process);

  XBT_INFO("Process(pid=%d) is %ssuspended", MSG_process_get_PID(process),
           (MSG_process_is_suspended(process)) ? "" : "not ");
  MSG_process_sleep(2);

  XBT_INFO("Resume Process(pid=%d)", MSG_process_get_PID(process));
  MSG_process_resume(process);

  XBT_INFO("Process(pid=%d) is %ssuspended", MSG_process_get_PID(process),
           (MSG_process_is_suspended(process)) ? "" : "not ");
  MSG_process_sleep(2);
  MSG_process_kill(process);

  XBT_INFO("Goodbye now!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\t Example: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("master", master, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("slave", slave, NULL, MSG_get_host_by_name("Tremblay"));

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
