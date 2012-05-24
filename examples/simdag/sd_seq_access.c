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
  SD_task_t taskA, taskB, taskC;
  xbt_dynar_t changed_tasks;

  /* initialisation of SD */
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

  /* Change the access mode of the workstations */
  workstations = SD_workstation_get_list();
  for (i = 0; i < 2; i++) {
  	SD_workstation_dump(workstations[i]);
  	
    SD_workstation_set_access_mode(workstations[i],
                                   SD_WORKSTATION_SEQUENTIAL_ACCESS);
    XBT_INFO(" Change access mode of %s to %s",
          SD_workstation_get_name(workstations[i]),
          (SD_workstation_get_access_mode(workstations[i]) ==
           SD_WORKSTATION_SEQUENTIAL_ACCESS) ? "sequential" : "shared");
  }

  /* creation of the tasks and their dependencies */
  taskA = SD_task_create_comp_seq("Task A", NULL, 2e10);
  taskB = SD_task_create_comm_e2e("Task B", NULL, 2e8);
  taskC = SD_task_create_comp_seq("Task C", NULL, 1e10);

  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskA, SD_RUNNING);
  SD_task_watch(taskB, SD_RUNNING);
  SD_task_watch(taskC, SD_RUNNING);
  SD_task_watch(taskC, SD_DONE);


  /* scheduling parameters */
  SD_task_schedulel(taskA, 1, workstations[0]);
  SD_task_schedulel(taskB, 2, workstations[0], workstations[1]);
  SD_task_schedulel(taskC, 1, workstations[1]);

  /* let's launch the simulation! */
  while (!xbt_dynar_is_empty(changed_tasks = SD_simulate(-1.0))) {
  	XBT_INFO(" Simulation was suspended, check workstation states"); 
    for (i = 0; i < 2; i++) {
	  SD_workstation_dump(workstations[i]);
    }
    xbt_dynar_free(&changed_tasks);
  }
  xbt_dynar_free(&changed_tasks);

  XBT_DEBUG("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);

  XBT_DEBUG("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}
