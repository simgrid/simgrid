/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static int master(int argc, char *argv[])
{
  int slaves_count = 0;
  msg_host_t *slaves = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;
  int i;
  XBT_ATTRIB_UNUSED int read;

  read = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  read = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(read, "Invalid argument %s\n", argv[2]);
  read = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(read, "Invalid argument %s\n", argv[3]);

  {                             /* Process organization */
    slaves_count = argc - 4;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_host_by_name(argv[i]);
      if (slaves[i - 4] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  XBT_INFO("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    XBT_INFO("%s", MSG_host_get_name(slaves[i]));

  XBT_INFO("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    msg_task_t task = MSG_task_create("Task", task_comp_size, task_comm_size, xbt_new0(double, 1));
    *((double *) task->data) = MSG_get_clock();

    msg_error_t a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i % slaves_count]),10.0);

    if (a == MSG_OK) {
      XBT_INFO("Send completed");
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      free(task->data);
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!",
               MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die( "Unexpected behavior");
    }
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    msg_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);
    int a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i]),1.0);
    if (a == MSG_OK)
      continue;
    if (a == MSG_HOST_FAILURE) {
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Can't reach '%s'! Nevermind. Let's keep going!", MSG_host_get_name(slaves[i]));
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
               MSG_host_get_name(slaves[i % slaves_count]));
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior with '%s': %d", MSG_host_get_name(slaves[i]), a);
    }
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  return 0;
}

static int slave(int argc, char *argv[])
{
  while (1) {
    msg_task_t task = NULL;
    int a;
    double time1, time2;

    time1 = MSG_get_clock();
    a = MSG_task_receive( &(task), MSG_host_get_name(MSG_host_self()) );
    time2 = MSG_get_clock();
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      if (time1 < *((double *) task->data))
        time1 = *((double *) task->data);
      XBT_INFO("Communication time : \"%f\"", time2 - time1);
      XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
      a = MSG_task_execute(task);
      if (a == MSG_OK) {
        XBT_INFO("\"%s\" done", MSG_task_get_name(task));
        free(task->data);
        MSG_task_destroy(task);
      } else if (a == MSG_HOST_FAILURE) {
        XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
        free(task->data);
        MSG_task_destroy(task);
        return 0;
      } else {
        XBT_INFO("Hey ?! What's up ? ");
        xbt_die("Unexpected behavior");
      }
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior");
    }
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

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
