/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_typed_tasks_test,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  int i;
  unsigned int ctr;
  const char *platform_file;
  const SD_workstation_t *workstations;
  SD_task_t task, taskA, taskB, taskC, taskD, taskE;
  xbt_dynar_t changed_tasks;

  double computation_amount[4];
  double communication_amount[16] = { 0 };
  SD_workstation_t workstation_list[4];
  
  /* initialization of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    XBT_INFO("Usage: %s platform_file", argv[0]);
    XBT_INFO("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  platform_file = argv[1];
  SD_create_environment(platform_file);
 
  workstations = SD_workstation_get_list();

  for (i=0;i<4;i++)
    XBT_INFO("%s runs at %f flops", SD_workstation_get_name(workstations[i]),
	     SD_workstation_get_power(workstations[i]));

  /* creation of some typed tasks and their dependencies */
  taskA = SD_task_create_comp_seq("Task A", NULL, 1e9);
  taskB = SD_task_create_comm_e2e("Task B", NULL, 1e7);
  taskC = SD_task_create_comp_seq("Task C", NULL, 1e9);
  taskD = SD_task_create_comp_par_amdahl("Task D", NULL, 1e9, 0.2);
  taskE = SD_task_create("Task E", NULL, 1e9);

  double toto = (0.2 + (1 - 0.2)/4) * 1e9;
  XBT_INFO("%f %f",toto ,toto/SD_workstation_get_power(workstations[0]));

  SD_task_dependency_add(NULL, NULL, taskA, taskB);
  SD_task_dependency_add(NULL, NULL, taskB, taskC);

  SD_task_schedulel(taskA, 1, workstations[8]);
  SD_task_schedulel(taskC, 1, workstations[9]);

  SD_task_distribute_comp_amdhal(taskD, 4);
  SD_task_schedulev(taskD, 4, workstations);

  for (i=0;i<4;i++){
    workstation_list[i]=workstations[i+4];
    computation_amount[i]=toto;
  }
  SD_task_schedule(taskE, 4, workstation_list,
                   computation_amount, communication_amount, -1);

  changed_tasks = SD_simulate(-1.0);
  xbt_dynar_foreach(changed_tasks, ctr, task) {
    XBT_INFO("Task '%s' start time: %f, finish time: %f",
          SD_task_get_name(task),
          SD_task_get_start_time(task), SD_task_get_finish_time(task));
  }

  XBT_DEBUG("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);
  SD_task_destroy(taskE);

  XBT_DEBUG("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}
