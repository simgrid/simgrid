/* Copyright (c) 2007-2010, 2013. The SimGrid Team.
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

static int dvfs(int argc, char *argv[]);
static int process_code(int argc, char *argv[]);

static int process_code(int argc, char *argv[])
{
  msg_task_t task1 = NULL;
  double cpu_task = 0;
  double task_time = MSG_get_clock();

  if (argc == 2)
  {
	  /* Run a sleep task */
	  double sleep_task = atof(argv[1]);

	  MSG_process_sleep(sleep_task);
	  task_time = MSG_get_clock() - task_time;
	  XBT_INFO("Process %s executed task sleep cpu=%f, duration = %f",
			  MSG_process_get_name(MSG_process_self()), 0.0, task_time);
	  XBT_INFO("==================================================");
 }


  // Run a task
  cpu_task = atof(argv[0]);
  task1 = MSG_task_create ("task", cpu_task, 0, NULL);
  MSG_task_execute (task1);
  MSG_task_destroy(task1);

  task_time = MSG_get_clock() - task_time;
  XBT_INFO("Process %s executed task cpu=%f, duration = %f",
		  MSG_process_get_name(MSG_process_self()), cpu_task, task_time);
  XBT_INFO("==================================================");
  return 0;
}


static int dvfs(int argc, char *argv[])
{
  msg_host_t host = NULL;
  double task_time = 0;
  host = MSG_host_self();

  double current_peak = MSG_get_host_current_power_peak(host);

  XBT_INFO("Current power peak=%f", current_peak);
  double consumed_energy = MSG_get_host_consumed_energy(host);
  XBT_INFO("Total energy (Joules): %f", consumed_energy);

  // Process 1 - long CPU task
  int argc1 = 1;
  char** params1 = xbt_malloc0(sizeof(char *) * argc1);
  params1[0] = xbt_strdup("400.0E6");
  MSG_process_create_with_arguments("proc1", process_code, NULL, host, argc1, params1);

  // Process 2 - sleep 2 sec + CPU task
  int argc2 = 2;
  char** params2 = xbt_malloc0(sizeof(char *) * argc2);
  params2[0] = xbt_strdup("100.0E6");
  params2[1] = xbt_strdup("2");
  MSG_process_create_with_arguments("proc2", process_code, NULL, host, argc2, params2);

  // Process 3 - sleep 2 sec + CPU task
  int argc3 = 2;
  char** params3 = xbt_malloc0(sizeof(char *) * argc3);
  params3[0] = xbt_strdup("100.0E6");
  params3[1] = xbt_strdup("2");
  MSG_process_create_with_arguments("proc3", process_code, NULL, host, argc3, params3);


  // Main process
  MSG_process_sleep(8);

  task_time = MSG_get_clock() - task_time;
  XBT_INFO("Task simulation time: %e", task_time);
  consumed_energy = MSG_get_host_consumed_energy(host);
  XBT_INFO("Total energy (Joules): %f", consumed_energy);

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

