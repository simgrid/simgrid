/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_comm_throttling,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  unsigned int ctr;
  const char *platform_file;
  const SD_workstation_t *workstations;
  SD_task_t task, taskA, taskB, taskC, taskD, taskE;
  xbt_dynar_t changed_tasks;

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

  /* creation of some typed tasks and their dependencies */
  /* chain of five tasks, three compute tasks with two data transfers */
  /* in between */
  taskA = SD_task_create_comp_seq("Task A", NULL, 5e9);
  taskB = SD_task_create_comm_e2e("Task B", NULL, 1e7);
  taskC = SD_task_create_comp_seq("Task C", NULL, 5e9);
  taskD = SD_task_create_comm_e2e("Task D", NULL, 1e7);
  taskE = SD_task_create_comp_seq("Task E", NULL, 5e9);

  SD_task_dependency_add(NULL, NULL, taskA, taskB);
  SD_task_dependency_add(NULL, NULL, taskB, taskC);
  SD_task_dependency_add(NULL, NULL, taskC, taskD);
  SD_task_dependency_add(NULL, NULL, taskD, taskE);

  /* Add watchpoints on completion of compute tasks */
  SD_task_watch(taskA, SD_DONE);
  SD_task_watch(taskC, SD_DONE);
  SD_task_watch(taskE, SD_DONE);

  /* Auto-schedule the compute tasks on three different workstations */
  /* Data transfer tasks taskB and taskD are automagically scheduled */
  SD_task_schedulel(taskA, 1, workstations[0]);
  SD_task_schedulel(taskC, 1, workstations[1]);
  SD_task_schedulel(taskE, 1, workstations[0]);
  while (!xbt_dynar_is_empty((changed_tasks = SD_simulate(-1.0)))) {
    XBT_INFO("Simulation stopped after %.4f seconds", SD_get_clock());
    xbt_dynar_foreach(changed_tasks, ctr, task) {
      XBT_INFO("Task '%s' start time: %f, finish time: %f",
         SD_task_get_name(task),
         SD_task_get_start_time(task), 
         SD_task_get_finish_time(task));
 
    }
    /* let throttle the communication for taskD if its parent is SD_DONE */
    if (SD_task_get_state(taskC) == SD_DONE)
      SD_task_set_rate(taskD, 0.5);
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
