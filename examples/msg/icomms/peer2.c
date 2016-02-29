/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>msg/icomms/peer2.c</b>: demonstrates the @ref MSG_comm_waitall function
 */

static int sender(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");

  msg_comm_t *comm = xbt_new(msg_comm_t, number_of_tasks + receivers_count);
  int i;
  msg_task_t task = NULL;
  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    char sprintf_buffer[256];
    sprintf(mailbox, "receiver-%ld", i % receivers_count);
    sprintf(sprintf_buffer, "Task_%d", i);
    task =
        MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                        NULL);
    comm[i] = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
  }
  for (i = 0; i < receivers_count; i++) {
    char mailbox[80];
    sprintf(mailbox, "receiver-%ld", i % receivers_count);
    task = MSG_task_create("finalize", 0, 0, 0);
    comm[i + number_of_tasks] = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld finalize", i % receivers_count);

  }
  /* Here we are waiting for the completion of all communications */
  MSG_comm_waitall(comm, (number_of_tasks + receivers_count), -1);
  for (i = 0; i < number_of_tasks + receivers_count; i++)
    MSG_comm_destroy(comm[i]);

  XBT_INFO("Goodbye now!");
  xbt_free(comm);
  return 0;
}

static int receiver(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED msg_error_t res;
  int id = -1;
  char mailbox[80];
  msg_comm_t res_irecv;
  XBT_ATTRIB_UNUSED int read;
  read = sscanf(argv[1], "%d", &id);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  MSG_process_sleep(10);
  sprintf(mailbox, "receiver-%d", id);
  while (1) {
    res_irecv = MSG_task_irecv(&(task), mailbox);
    XBT_INFO("Wait to receive a task");
    res = MSG_comm_wait(res_irecv, -1);
    MSG_comm_destroy(res_irecv);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("sender", sender);
  MSG_function_register("receiver", receiver);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
