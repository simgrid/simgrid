/* Copyright (c) 2006-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_comm_throttling, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  unsigned int ctr;
  SD_task_t task;
  xbt_dynar_t changed_tasks;

  SD_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\nExample: %s two_clusters.xml", argv[0], argv[0]);

  SD_create_environment(argv[1]);

  const sg_host_t *hosts = sg_host_list();

  /* creation of some typed tasks and their dependencies */
  /* chain of five tasks, three compute tasks with two data transfers in between */
  SD_task_t taskA = SD_task_create_comp_seq("Task A", NULL, 5e9);
  SD_task_t taskB = SD_task_create_comm_e2e("Task B", NULL, 1e7);
  SD_task_t taskC = SD_task_create_comp_seq("Task C", NULL, 5e9);
  SD_task_t taskD = SD_task_create_comm_e2e("Task D", NULL, 1e7);
  SD_task_t taskE = SD_task_create_comp_seq("Task E", NULL, 5e9);

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
  SD_task_schedulel(taskA, 1, hosts[0]);
  SD_task_schedulel(taskC, 1, hosts[1]);
  SD_task_schedulel(taskE, 1, hosts[0]);
  while (!xbt_dynar_is_empty((changed_tasks = SD_simulate(-1.0)))) {
    XBT_INFO("Simulation stopped after %.4f seconds", SD_get_clock());
    xbt_dynar_foreach(changed_tasks, ctr, task) {
      XBT_INFO("Task '%s' start time: %f, finish time: %f", SD_task_get_name(task), SD_task_get_start_time(task),
               SD_task_get_finish_time(task));
    }

    /* let throttle the communication for taskD if its parent is SD_DONE */
    /* the bandwidth is 1.25e8, the data size is 1e7, and we want to throttle the bandwidth by a factor 2.
     * The rate is then 1.25e8/(2*1e7)=6.25
     * Changing the rate is possible before the task execution starts (in SD_RUNNING state).
     */
    if (SD_task_get_state(taskC) == SD_DONE && SD_task_get_state(taskD) < SD_RUNNING)
      SD_task_set_rate(taskD, 6.25);
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
