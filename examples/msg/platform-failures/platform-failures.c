/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static int master(int argc, char *argv[])
{
  int i;

  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long workers_count = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    snprintf(mailbox, 255, "worker-%ld", i % workers_count);

    msg_task_t task = MSG_task_create("Task", task_comp_size, task_comm_size, xbt_new0(double, 1));
    *((double *) task->data) = MSG_get_clock();

    switch ( MSG_task_send_with_timeout(task,mailbox,10.0) ) {
    case MSG_OK:
      XBT_INFO("Send completed");
      break;

    case MSG_HOST_FAILURE:
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      free(task->data);
      MSG_task_destroy(task);
      return 0;

    case MSG_TRANSFER_FAILURE:
      XBT_INFO("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!", mailbox);
      free(task->data);
      MSG_task_destroy(task);
      break;

    case MSG_TIMEOUT:
      XBT_INFO ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox);
      free(task->data);
      MSG_task_destroy(task);
      break;

    default:
      xbt_die( "Unexpected behavior");
    }
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < workers_count; i++) {
    char mailbox[256];
    snprintf(mailbox, 255, "worker-%ld", i % workers_count);
    msg_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);

    switch (MSG_task_send_with_timeout(task,mailbox,1.0)) {
    case MSG_HOST_FAILURE:
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      MSG_task_destroy(task);
      break;

    case MSG_TRANSFER_FAILURE:
      XBT_INFO("Mmh. Can't reach '%s'! Nevermind. Let's keep going!", mailbox);
      MSG_task_destroy(task);
      break;

    case MSG_TIMEOUT:
      XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox);
      MSG_task_destroy(task);
      break;

    case MSG_OK:
      /* nothing */
      break;

    default:
      xbt_die("Unexpected behavior with '%s'", mailbox);
    }
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

static int worker(int argc, char *argv[])
{
  char mailbox[80];

  long id= xbt_str_parse_int(argv[1], "Invalid argument %s");

  snprintf(mailbox, 79,"worker-%ld", id);

  while (1) {
    double time1 = MSG_get_clock();
    msg_task_t task = NULL;
    int retcode = MSG_task_receive( &(task), mailbox);
    double time2 = MSG_get_clock();
    if (retcode == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      if (time1 < *((double *) task->data))
        time1 = *((double *) task->data);
      XBT_INFO("Communication time : \"%f\"", time2 - time1);
      XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
      retcode = MSG_task_execute(task);
      if (retcode == MSG_OK) {
        XBT_INFO("\"%s\" done", MSG_task_get_name(task));
        free(task->data);
        MSG_task_destroy(task);
      } else if (retcode == MSG_HOST_FAILURE) {
        XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
        free(task->data);
        MSG_task_destroy(task);
        return 0;
      } else {
        XBT_INFO("Hey ?! What's up ? ");
        xbt_die("Unexpected behavior");
      }
    } else if (retcode == MSG_HOST_FAILURE) {
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (retcode == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    } else {
      xbt_die("Unexpected behavior");
    }
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
