/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitany, "Messages specific for this msg example");

static int sender(int argc, char* argv[])
{
  xbt_assert(argc == 6, "This function expects 5 parameters from the XML deployment file");
  long number_of_tasks  = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count  = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");
  int diff_com          = xbt_str_parse_int(argv[5], "Invalid value for diff_comm: %s");

  xbt_dynar_t comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  /* First pack the communications in the dynar */
  for (int i = 0; i < number_of_tasks; i++) {
    double coef = (diff_com == 0) ? 1 : (i + 1);

    char mailbox[80];
    char taskname[80];
    snprintf(mailbox, 79, "receiver-%ld", (i % receivers_count));
    snprintf(taskname, 79, "Task_%d", i);
    msg_task_t task = MSG_task_create(taskname, task_comp_size, task_comm_size / coef, NULL);
    msg_comm_t comm = MSG_task_isend(task, mailbox);
    xbt_dynar_push_as(comms, msg_comm_t, comm);
    XBT_INFO("Send to receiver-%ld %s comm_size %f", i % receivers_count, taskname, task_comm_size / coef);
  }

  /* Here we are waiting for the completion of all communications */
  while (xbt_dynar_is_empty(comms) == 0) {
    msg_comm_t comm;
    xbt_dynar_remove_at(comms, MSG_comm_waitany(comms), &comm);
    MSG_comm_destroy(comm);
  }
  xbt_dynar_free(&comms);

  /* Here we are waiting for the completion of all tasks */
  for (int i = 0; i < receivers_count; i++) {
    msg_task_t task      = NULL;
    msg_comm_t comm      = MSG_task_irecv(&task, "finalize");
    msg_error_t res_wait = MSG_comm_wait(comm, -1);
    xbt_assert(res_wait == MSG_OK, "MSG_comm_wait failed");
    MSG_comm_destroy(comm);
    MSG_task_destroy(task);
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

static int receiver(int argc, char* argv[])
{
  xbt_assert(argc == 3, "This function expects 2 parameters from the XML deployment file");
  int id            = xbt_str_parse_int(argv[1], "ID should be numerical, not %s");
  int task_amount   = xbt_str_parse_int(argv[2], "Invalid amount of tasks: %s");
  msg_task_t* tasks = xbt_new(msg_task_t, task_amount);
  xbt_dynar_t comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);

  char mailbox[80];
  snprintf(mailbox, 79, "receiver-%d", id);

  MSG_process_sleep(10);
  for (int i = 0; i < task_amount; i++) {
    XBT_INFO("Wait to receive task %d", i);
    tasks[i]        = NULL;
    msg_comm_t comm = MSG_task_irecv(&tasks[i], mailbox);
    xbt_dynar_push_as(comms, msg_comm_t, comm);
  }

  /* Here we are waiting for the receiving of all communications */
  while (!xbt_dynar_is_empty(comms)) {
    msg_comm_t comm;
    // MSG_comm_waitany returns the rank of the comm that just ended. Remove it.
    xbt_dynar_remove_at(comms, MSG_comm_waitany(comms), &comm);
    msg_task_t task = MSG_comm_get_task(comm);
    MSG_comm_destroy(comm);
    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    msg_error_t err = MSG_task_destroy(task);
    xbt_assert(err == MSG_OK, "MSG_task_destroy failed");
  }
  xbt_dynar_free(&comms);
  xbt_free(tasks);

  /* Here we tell to sender that all tasks are done */
  MSG_task_send(MSG_task_create(NULL, 0, 0, NULL), "finalize");
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
