/* Copyright (c) 2007-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include<stdio.h>

#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc */
#include "simgrid/plugins.h"

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
  host = MSG_get_host_by_name("MyHost1");


  XBT_INFO("Energetic profile: %s",
		  MSG_host_get_property_value(host,"watt_per_state"));
  XBT_INFO("Initial power peak=%.0E flop/s; Consumed energy (Joules)=%.0E J",
		  MSG_get_host_current_power_peak(host), MSG_get_host_consumed_energy(host));

  double start = MSG_get_clock();
  XBT_INFO("Sleep for 10 seconds");
  MSG_process_sleep(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current power peak=%.0E; Current consumed energy=%.2f J",
		  MSG_get_clock()-start,
		  MSG_get_host_current_power_peak(host), MSG_get_host_consumed_energy(host));

  // Run a task
  start = MSG_get_clock();
  XBT_INFO("Run a task for 100E6 flops");
  task1 = MSG_task_create ("t1", 100E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);
  XBT_INFO("Task done (duration: %.2f s). Current power peak=%.0E flop/s; Current consumed energy=%.0f J",
		  MSG_get_clock()-start,
		  MSG_get_host_current_power_peak(host), MSG_get_host_consumed_energy(host));

  // ========= Change power peak =========
  int pstate=2;
  MSG_set_host_power_peak_at(host, pstate);
  XBT_INFO("========= Requesting pstate %d (power should be of %.2f flop/s and is of %.2f flop/s",
		  pstate,
		  MSG_get_host_power_peak_at(host, pstate),
		  MSG_get_host_current_power_peak(host));

  // Run a second task
  start = MSG_get_clock();
  XBT_INFO("Run a task for 100E6 flops");
  task1 = MSG_task_create ("t2", 100E6, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);
  XBT_INFO("Task done (duration: %.2f s). Current power peak=%.0E flop/s; Current consumed energy=%.0f J",
		  MSG_get_clock()-start,
		  MSG_get_host_current_power_peak(host), MSG_get_host_consumed_energy(host));

  start = MSG_get_clock();
  XBT_INFO("Sleep for 10 seconds");
  MSG_process_sleep(4);
  XBT_INFO("Done sleeping (duration: %.2f s). Current power peak=%.0E flop/s; Current consumed energy=%.0f J",
		  MSG_get_clock()-start,
		  MSG_get_host_current_power_peak(host), MSG_get_host_consumed_energy(host));

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  sg_energy_plugin_init();
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

  XBT_INFO("Total simulation time: %.2f", MSG_get_clock());

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}

