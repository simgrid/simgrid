/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitall, "Messages specific for this msg example");

static int sender(int argc, char* argv[])
{
  xbt_assert(argc == 5, "This function expects 4 parameters from the XML deployment file");
  long number_of_tasks  = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count  = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");

  msg_comm_t* comm = xbt_new(msg_comm_t, number_of_tasks + receivers_count);
  for (int i = 0; i < number_of_tasks; i++) {
    char mailbox[80];
    char taskname[80];
    snprintf(mailbox, 79, "receiver-%ld", i % receivers_count);
    snprintf(taskname, 79, "Task_%d", i);
    msg_task_t task = MSG_task_create(taskname, task_comp_size, task_comm_size, NULL);
    comm[i]         = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
  }
  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    snprintf(mailbox, 79, "receiver-%ld", i % receivers_count);
    msg_task_t task           = MSG_task_create("finalize", 0, 0, 0);
    comm[i + number_of_tasks] = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld finalize", i % receivers_count);
  }

  /* Here we are waiting for the completion of all communications */
  MSG_comm_waitall(comm, (number_of_tasks + receivers_count), -1);
  for (int i = 0; i < number_of_tasks + receivers_count; i++)
    MSG_comm_destroy(comm[i]);

  XBT_INFO("Goodbye now!");
  xbt_free(comm);
  return 0;
}

static int receiver(int argc, char* argv[])
{
  xbt_assert(argc == 2, "This function expects 1 parameter from the XML deployment file");
  int id = xbt_str_parse_int(argv[1], "Any process of this example must have a numerical name, not %s");

  char mailbox[80];
  snprintf(mailbox, 79, "receiver-%d", id);

  MSG_process_sleep(10);
  while (1) {
    XBT_INFO("Wait to receive a task");
    msg_task_t task = NULL;
    msg_comm_t comm = MSG_task_irecv(&task, mailbox);
    msg_error_t res = MSG_comm_wait(comm, -1);
    xbt_assert(res == MSG_OK, "MSG_task_get failed");
    MSG_comm_destroy(comm);
    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (strcmp(MSG_task_get_name(task), "finalize") == 0) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("sender", sender);
  MSG_function_register("receiver", receiver);
  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
