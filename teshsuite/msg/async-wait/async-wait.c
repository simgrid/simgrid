/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_wait, "Messages specific for this msg example");

/* Main function of the Sender process */
static int sender(int argc, char* argv[])
{
  xbt_assert(argc == 7, "The sender function expects 6 arguments from the XML deployment file");
  long number_of_tasks    = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");       /* - number of tasks */
  double task_comp_size   = xbt_str_parse_double(argv[2], "Invalid computational size: %s"); /* - computational cost */
  double task_comm_size   = xbt_str_parse_double(argv[3], "Invalid communication size: %s"); /* - communication cost */
  long receivers_count    = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");   /* - number of receivers */
  double sleep_start_time = xbt_str_parse_double(argv[5], "Invalid sleep start time: %s");   /* - start time */
  double sleep_test_time  = xbt_str_parse_double(argv[6], "Invalid test time: %s");          /* - test time */

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);

  MSG_process_sleep(sleep_start_time);
  for (int i = 0; i < number_of_tasks; i++) {
    char mailbox[80];
    char taskname[80];

    snprintf(mailbox, 79, "receiver-%ld", i % receivers_count);
    snprintf(taskname, 79, "Task_%d", i);

    /* This process first creates a task and send it asynchronously with @ref MSG_task_isend. Then, if: */
    msg_task_t task = MSG_task_create(taskname, task_comp_size, task_comm_size, NULL);
    msg_comm_t comm = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);

    if (sleep_test_time > 0) {           /* - "test_time" is set to 0, wait on @ref MSG_comm_wait */
      while (MSG_comm_test(comm) == 0) { /* - Call @ref MSG_comm_test every "test_time" otherwise */
        MSG_process_sleep(sleep_test_time);
      }
    } else {
      MSG_comm_wait(comm, -1);
    }
    MSG_comm_destroy(comm);
  }

  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    snprintf(mailbox, 79, "receiver-%d", i);
    msg_task_t task = MSG_task_create("finalize", 0, 0, 0);
    msg_comm_t comm = MSG_task_isend(task, mailbox);
    XBT_INFO("Send to receiver-%d finalize", i);
    if (sleep_test_time > 0) {
      while (MSG_comm_test(comm) == 0) {
        MSG_process_sleep(sleep_test_time);
      }
    } else {
      MSG_comm_wait(comm, -1);
    }
    MSG_comm_destroy(comm);
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

/* Receiver process expects 3 arguments: */
static int receiver(int argc, char* argv[])
{
  xbt_assert(argc == 4, "The relay_runner function does not accept any parameter from the XML deployment file");
  int id                  = xbt_str_parse_int(argv[1], "Invalid id: %s");                       /* - unique id */
  double sleep_start_time = xbt_str_parse_double(argv[2], "Invalid sleep start parameter: %s"); /* - start time */
  double sleep_test_time  = xbt_str_parse_double(argv[3], "Invalid sleep test parameter: %s");  /* - test time */
  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);

  MSG_process_sleep(sleep_start_time); /* This process first sleeps for "start time" seconds.  */

  char mailbox[80];
  snprintf(mailbox, 79, "receiver-%d", id);
  while (1) {
    msg_task_t task = NULL;
    msg_comm_t comm = MSG_task_irecv(&task, mailbox); /* Then it posts asynchronous receives (@ref MSG_task_irecv) and*/
    XBT_INFO("Wait to receive a task");

    if (sleep_test_time > 0) {           /* - if "test_time" is set to 0, wait on @ref MSG_comm_wait */
      while (MSG_comm_test(comm) == 0) { /* - Call @ref MSG_comm_test every "test_time" otherwise */
        MSG_process_sleep(sleep_test_time);
      }
    } else {
      msg_error_t res = MSG_comm_wait(comm, -1);
      xbt_assert(res == MSG_OK, "MSG_task_get failed");
    }
    MSG_comm_destroy(comm);

    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (strcmp(MSG_task_get_name(task), "finalize") == 0) { /* If the received task is "finalize", the process ends */
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task)); /* Otherwise, the task is processed */
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

  MSG_create_environment(argv[1]); /* - Load the platform description */

  MSG_function_register("sender", sender);
  MSG_function_register("receiver", receiver);
  MSG_launch_application(argv[2]); /* - Deploy the sender and receiver processes */

  msg_error_t res = MSG_main(); /* - Run the simulation */

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
