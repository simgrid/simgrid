/* Copyright (c) 2009-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

const char* mailbox = "mailbox";
#define task_comp_size 1000
#define task_comm_size 100000

static int my_onexit(smx_process_exit_status_t status, int *pid){
  XBT_INFO("Process \"%d\" killed.", *pid);
  return 0;
}

static int sendpid(int argc, char *argv[])
{
  int pid = MSG_process_self_PID();
  MSG_process_on_exit((int_f_pvoid_pvoid_t)my_onexit, &pid);
  msg_task_t task = MSG_task_create("pid", task_comp_size, task_comm_size, &pid);
  XBT_INFO("Sending pid of \"%d\".", pid);
  MSG_task_send(task, mailbox);
  XBT_INFO("Send of pid \"%d\" done.", pid);
  MSG_process_suspend(MSG_process_self());
  return 0;
}

static int killall(int argc, char *argv[]){
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED int res;

  for (int i=0; i<3;i++) {
    res = MSG_task_receive(&(task), mailbox);
    int pid = *(int*)MSG_task_get_data(task);
    MSG_task_destroy(task);
    XBT_INFO("Killing process \"%d\".", pid);
    MSG_process_kill(MSG_process_from_PID(pid));
    task = NULL;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  MSG_process_killall(atoi(argv[2]));

  MSG_create_environment(argv[1]);
  MSG_process_create("sendpid", sendpid, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("sendpid", sendpid, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("sendpid", sendpid, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("killall", killall, NULL, MSG_get_host_by_name("Tremblay"));

  res = MSG_main();

  return res != MSG_OK;
}
