/* Copyright (c) 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

/* simple test trying to load a Parallel Task Graph (PTG) as a DOT file.    */
int main(int argc, char **argv){
  xbt_dynar_t dot;
  unsigned int cursor;
  SD_task_t task;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  xbt_assert (argc > 1,"Usage: %s platform_file dot_file example: %s ../2clusters.xml ptg.dot", argv[0], argv[0]);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DOT file */
  dot = SD_PTG_dotload(argv[2]);
  if(dot == NULL){
    SD_exit();
    xbt_die("No dot load may be you have a cycle in your graph");
  }

  /* Display all the tasks */
  XBT_INFO("------------------- Display all tasks of the loaded DAG ---------------------------");
  xbt_dynar_foreach(dot, cursor, task) {
    SD_task_dump(task);
  }

  FILE *dotout = fopen("dot.dot", "w");
  fprintf(dotout, "digraph A {\n");
  xbt_dynar_foreach(dot, cursor, task) {
    SD_task_dotty(task, dotout);
  }
  fprintf(dotout, "}\n");
  fclose(dotout);

  /* Schedule them all on all the first workstation */
  XBT_INFO("------------------- Schedule tasks ---------------------------");
  const sg_host_t *ws_list = sg_host_list();
  int count = sg_host_count();
  xbt_dynar_foreach(dot, cursor, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL) {
        SD_task_schedulev(task, count, ws_list);
    }
  }

  XBT_INFO("------------------- Run the schedule ---------------------------");
  SD_simulate(-1);
  XBT_INFO("Makespan: %f", SD_get_clock());
  xbt_dynar_foreach(dot, cursor, task) {
    SD_task_destroy(task);
  }
  xbt_dynar_free_container(&dot);

  /* exit */
  SD_exit();
  return 0;
}
