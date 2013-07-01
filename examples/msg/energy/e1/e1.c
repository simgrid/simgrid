/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include<stdio.h>

#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int dvfs(int argc, char *argv[]);


int dvfs(int argc, char *argv[])
{
  msg_host_t host = NULL;
  msg_task_t task1 = NULL;
  double task_time = 0;
  host = MSG_get_host_by_name("MyHost1");

  XBT_INFO("dvfs start");

  int nb = MSG_get_host_nb_pstates(host);
  XBT_INFO("Number of Processor states=%d", nb);

  double current_peak = MSG_get_host_current_power_peak(host);
  XBT_INFO("Current power peak=%lf", current_peak);

  // Run a task
  task1 = MSG_task_create ("t1", 100E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);

  task_time = MSG_get_clock();
  XBT_INFO("Task1 simulation time: %le", task_time);

  // ========= Change power peak =========
  int peak_index=2;
  double peak_at = MSG_get_host_power_peak_at(host, peak_index);
  XBT_INFO("=========Changing power peak value to %lf (at index %d)", peak_at, peak_index);

  MSG_set_host_power_peak_at(host, peak_index);

  current_peak = MSG_get_host_current_power_peak(host);
  XBT_INFO("Current power peak=%lf", current_peak);

  // Run a second task
  task1 = MSG_task_create ("t1", 100E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);

  task_time = MSG_get_clock() - task_time;
  XBT_INFO("Task2 simulation time: %le", task_time);

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);

  if (argc != 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n",
              argv[0]);
    XBT_CRITICAL
        ("example: %s msg_platform.xml msg_deployment.xml\n",
         argv[0]);
    exit(1);
  }

  MSG_create_environment(argv[1]);

  /*   Application deployment */
  MSG_function_register("dvfs_test", dvfs);

  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Total simulation time: %le", MSG_get_clock());

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}

