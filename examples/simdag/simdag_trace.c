/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_seq_access,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  int i;
  const char *platform_file;
  const SD_workstation_t *workstations;
  int kind;
  SD_task_t task, taskA, taskB, taskC;
  xbt_dynar_t changed_tasks;
  SD_workstation_t workstation_list[2];
  double computation_amount[2];
  double communication_amount[4] = { 0 };
  double rate = -1.0;
  SD_workstation_t w1, w2;

  /* initialisation of SD */
  SD_init(&argc, argv);

  TRACE_start ();

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  platform_file = argv[1];
  SD_create_environment(platform_file);

  /* Change the access mode of the workstations */
  workstations = SD_workstation_get_list();
  w1 = workstations[0];
  w2 = workstations[1];
  for (i = 0; i < 2; i++) {
    SD_workstation_set_access_mode(workstations[i],
                                   SD_WORKSTATION_SEQUENTIAL_ACCESS);
    INFO2("Access mode of %s is %s",
          SD_workstation_get_name(workstations[i]),
          (SD_workstation_get_access_mode(workstations[i]) ==
           SD_WORKSTATION_SEQUENTIAL_ACCESS) ? "sequential" : "shared");
  }

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

  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskA, SD_RUNNING);
  SD_task_watch(taskB, SD_RUNNING);
  SD_task_watch(taskC, SD_RUNNING);
  SD_task_watch(taskC, SD_DONE);


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
  while (xbt_dynar_length(changed_tasks = SD_simulate(-1.0)) > 0) {
    for (i = 0; i < 2; i++) {
      task = SD_workstation_get_current_task(workstations[i]);
      if (task)
        kind = SD_task_get_kind(task);
      else {
        INFO1("There is no task running on %s",
              SD_workstation_get_name(workstations[i]));
        continue;
      }

      switch (kind) {
      case SD_TASK_COMP_SEQ:
        INFO2("%s is currently running on %s (SD_TASK_COMP_SEQ)",
              SD_task_get_name(task),
              SD_workstation_get_name(workstations[i]));
        break;
      case SD_TASK_COMM_E2E:
        INFO2("%s is currently running on %s (SD_TASK_COMM_E2E)",
              SD_task_get_name(task),
              SD_workstation_get_name(workstations[i]));
        break;
      case SD_TASK_NOT_TYPED:
        INFO1("Task running on %s has no type",
              SD_workstation_get_name(workstations[i]));
        break;
      default:
        ERROR0("Shouldn't be here");
      }
    }
    xbt_dynar_free_container(&changed_tasks);
  }

  DEBUG0("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);

  DEBUG0("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  TRACE_end();
  return 0;
}
