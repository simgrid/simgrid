/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_seq_access,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  const char *platform_file;
  const SD_workstation_t *workstations;
  SD_task_t taskA, taskB, taskC;
  SD_workstation_t workstation_list[2];
  double computation_amount[2];
  double communication_amount[4] = { 0 };
  double rate = -1.0;
  SD_workstation_t w1, w2;

  /* SD initialization */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  xbt_assert(argc > 1, "Usage: %s platform_file\n"
	     "\nExample: %s two_clusters.xml", argv[0], argv[0]);

  /* creation of the environment */
  platform_file = argv[1];
  SD_create_environment(platform_file);

  /* Change the access mode of the workstations */
  workstations = SD_workstation_get_list();
  w1 = workstations[0];
  w2 = workstations[1];

  /* creation of the tasks and their dependencies */
  taskA = SD_task_create_comp_seq("Task A", NULL, 2e9);
  taskB = SD_task_create_comm_e2e("Task B", NULL, 2e9);
  taskC = SD_task_create_comp_seq("Task C", NULL, 1e9);
  TRACE_category ("taskA");
  TRACE_category ("taskB");
  TRACE_category ("taskC");
  TRACE_sd_set_task_category (taskA, "taskA");
  TRACE_sd_set_task_category (taskB, "taskB");
  TRACE_sd_set_task_category (taskC, "taskC");

  /* scheduling parameters */
  workstation_list[0] = w1;
  workstation_list[1] = w2;
  computation_amount[0] = SD_task_get_amount(taskA);
  computation_amount[1] = SD_task_get_amount(taskB);

  communication_amount[1] = SD_task_get_amount(taskC);
  communication_amount[2] = 0.0;

  SD_task_schedule(taskA, 1, &w1,
                   &(computation_amount[0]), SD_SCHED_NO_COST, rate);
  SD_task_schedule(taskB, 2, workstation_list,
                   SD_SCHED_NO_COST, communication_amount, rate);
  SD_task_schedule(taskC, 1, &w1,
                   &(computation_amount[1]), SD_SCHED_NO_COST, rate);

  /* let's launch the simulation! */
  SD_simulate(-1.0);

  XBT_DEBUG("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);

  XBT_DEBUG("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}
