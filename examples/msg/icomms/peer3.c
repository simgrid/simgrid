/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>msg/icomms/peer3.c</b>: demonstrates the @ref MSG_comm_waitany function
 */

static int sender(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");
  int diff_com = xbt_str_parse_int(argv[5], "Invalid value for diff_comm: %s");
  double coef = 0;
  xbt_dynar_t d = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  int i;
  msg_task_t task;
  char mailbox[256];
  char sprintf_buffer[256];
  msg_comm_t comm;

  for (i = 0; i < number_of_tasks; i++) {
    if (diff_com == 0)
      coef = 1;
    else
      coef = (i + 1);

    sprintf(mailbox, "receiver-%ld", (i % receivers_count));
    sprintf(sprintf_buffer, "Task_%d", i);
    task =
        MSG_task_create(sprintf_buffer, task_comp_size,
                        task_comm_size / coef, NULL);
    comm = MSG_task_isend(task, mailbox);
    xbt_dynar_push_as(d, msg_comm_t, comm);
    XBT_INFO("Send to receiver-%ld %s comm_size %f", i % receivers_count, sprintf_buffer, task_comm_size / coef);
  }
  /* Here we are waiting for the completion of all communications */

  while (!xbt_dynar_is_empty(d)) {
    xbt_dynar_remove_at(d, MSG_comm_waitany(d), &comm);
    MSG_comm_destroy(comm);
  }
  xbt_dynar_free(&d);

  /* Here we are waiting for the completion of all tasks */
  sprintf(mailbox, "finalize");

  msg_comm_t res_irecv;
  XBT_ATTRIB_UNUSED msg_error_t res_wait;
  for (i = 0; i < receivers_count; i++) {
    task = NULL;
    res_irecv = MSG_task_irecv(&(task), mailbox);
    res_wait = MSG_comm_wait(res_irecv, -1);
    xbt_assert(res_wait == MSG_OK, "MSG_comm_wait failed");
    MSG_comm_destroy(res_irecv);
    MSG_task_destroy(task);
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

static int receiver(int argc, char *argv[])
{
  int id = -1;
  char mailbox[80];
  xbt_dynar_t comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  int tasks = xbt_str_parse_int(argv[2], "Invalid amount of tasks: %s");
  msg_task_t *task = xbt_new(msg_task_t, tasks);

  XBT_ATTRIB_UNUSED int read;
  read = sscanf(argv[1], "%d", &id);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  sprintf(mailbox, "receiver-%d", id);
  MSG_process_sleep(10);
  msg_comm_t res_irecv;
  for (int i = 0; i < tasks; i++) {
    XBT_INFO("Wait to receive task %d", i);
    task[i] = NULL;
    res_irecv = MSG_task_irecv(&task[i], mailbox);
    xbt_dynar_push_as(comms, msg_comm_t, res_irecv);
  }

  /* Here we are waiting for the receiving of all communications */
  msg_task_t task_com;
  while (!xbt_dynar_is_empty(comms)) {
    XBT_ATTRIB_UNUSED msg_error_t err;
    xbt_dynar_remove_at(comms, MSG_comm_waitany(comms), &res_irecv);
    task_com = MSG_comm_get_task(res_irecv);
    MSG_comm_destroy(res_irecv);
    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task_com));
    MSG_task_execute(task_com);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task_com));
    err = MSG_task_destroy(task_com);
    xbt_assert(err == MSG_OK, "MSG_task_destroy failed");
  }
  xbt_dynar_free(&comms);
  xbt_free(task);

  /* Here we tell to sender that all tasks are done */
  sprintf(mailbox, "finalize");
  res_irecv = MSG_task_isend(MSG_task_create(NULL, 0, 0, NULL), mailbox);
  MSG_comm_wait(res_irecv, -1);
  MSG_comm_destroy(res_irecv);
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
