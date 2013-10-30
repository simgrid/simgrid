/* Copyright (c) 2007-2010, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"


/** @addtogroup MSG_examples
 *
 * - <b>energy/e1/e1.c</b> Shows how a set of pstates can be defined
 * 		for a host and how the current pstate can be accessed/changed
 * 		with @ref MSG_get_host_current_power_peak and @ref
 * 		MSG_set_host_power_peak_at.
 * 		Make sure to read the platform XML file for details on how
 * 		to declare the CPU capacity for each pstate.
 *
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Pstate properties test");

int dvfs(int argc, char *argv[]);


int dvfs(int argc, char *argv[])
{
  msg_host_t host = NULL;
  msg_task_t task1 = NULL;
  double task_time = 0;
  double workload = 100E6;
  int new_peak_index=2;
  host = MSG_host_self();; //MSG_get_host_by_name("MyHost1");

  int nb = MSG_get_host_nb_pstates(host);
  XBT_INFO("Number of Processor states=%d", nb);

  double current_peak = MSG_get_host_current_power_peak(host);
  XBT_INFO("Current power peak=%f", current_peak);

  // Run a task
  task1 = MSG_task_create ("t1", workload, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);

  task_time = MSG_get_clock();
  XBT_INFO("Task1 simulation time: %e", task_time);

  // Change power peak
  if ((new_peak_index >= nb) || (new_peak_index < 0))
	  {
	  XBT_INFO("Cannot set pstate %d, host supports only %d pstates", new_peak_index, nb);
	  return 0;
	  }

  double peak_at = MSG_get_host_power_peak_at(host, new_peak_index);
  XBT_INFO("Changing power peak value to %f (at index %d)", peak_at, new_peak_index);

  MSG_set_host_power_peak_at(host, new_peak_index);

  current_peak = MSG_get_host_current_power_peak(host);
  XBT_INFO("Current power peak=%f", current_peak);

  // Run a second task
  task1 = MSG_task_create ("t1", workload, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);

  task_time = MSG_get_clock() - task_time;
  XBT_INFO("Task2 simulation time: %e", task_time);


  // Verify the default pstate is set to 0
  host = MSG_get_host_by_name("MyHost2");
  int nb2 = MSG_get_host_nb_pstates(host);
  XBT_INFO("Number of Processor states=%d", nb2);

  double current_peak2 = MSG_get_host_current_power_peak(host);
  XBT_INFO("Current power peak=%f", current_peak2);
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

  XBT_INFO("Total simulation time: %e", MSG_get_clock());

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}

