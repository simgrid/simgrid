/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * @section MSG_ex_icomms Asynchronous communications
 * 
 * There is several examples of asynchronous communications coming in
 * the archive. In addition to the fully documented example \ref
 * MSG_ex_asynchronous_communications, there is several other
 * examples in the archive:
 * 
 * - <b>msg/icomms/peer.c</b>: basic example of async functions (@ref MSG_task_isend, @ref MSG_task_irecv, @ref MSG_comm_wait)
 */

static int sender(int argc, char *argv[])
{
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid compute size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");
  double sleep_start_time = xbt_str_parse_double(argv[5], "Invalid sleep start time: %s");
  double sleep_test_time = xbt_str_parse_double(argv[6], "Invalid test time: %s");

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);

  msg_comm_t comm = NULL;
  int i;
  msg_task_t task = NULL;
  MSG_process_sleep(sleep_start_time);
  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    char sprintf_buffer[256];

    sprintf(mailbox, "receiver-%ld", i % receivers_count);
    sprintf(sprintf_buffer, "Task_%d", i);

    task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    comm = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);

    if (sleep_test_time == 0) {
      MSG_comm_wait(comm, -1);
    } else {
      while (MSG_comm_test(comm) == 0) {
        MSG_process_sleep(sleep_test_time);
      };
    }
    MSG_comm_destroy(comm);
  }

  for (i = 0; i < receivers_count; i++) {
    char mailbox[80];
    sprintf(mailbox, "receiver-%ld", i % receivers_count);
    task = MSG_task_create("finalize", 0, 0, 0);
    comm = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld finalize", i % receivers_count);
    if (sleep_test_time == 0) {
      MSG_comm_wait(comm, -1);
    } else {
      while (MSG_comm_test(comm) == 0) {
        MSG_process_sleep(sleep_test_time);
      };
    }
    MSG_comm_destroy(comm);
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

static int receiver(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED msg_error_t res;
  int id = -1;
  char mailbox[80];
  msg_comm_t res_irecv;
  double sleep_start_time = xbt_str_parse_double(argv[2], "Invalid sleep start parameter: %s");
  double sleep_test_time = xbt_str_parse_double(argv[3], "Invalid sleep test parameter: %s");
  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);

  XBT_ATTRIB_UNUSED int read;
  read = sscanf(argv[1], "%d", &id);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);

  MSG_process_sleep(sleep_start_time);

  sprintf(mailbox, "receiver-%d", id);
  while (1) {
    res_irecv = MSG_task_irecv(&(task), mailbox);
    XBT_INFO("Wait to receive a task");

    if (sleep_test_time == 0) {
      res = MSG_comm_wait(res_irecv, -1);
      xbt_assert(res == MSG_OK, "MSG_task_get failed");
    } else {
      while (MSG_comm_test(res_irecv) == 0) {
        MSG_process_sleep(sleep_test_time);
      };
    }
    MSG_comm_destroy(res_irecv);

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
