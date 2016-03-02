/* simple test trying to load a DOT file.                                   */

/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"
#include <string.h>
#include <libgen.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  xbt_dynar_t dot;
  unsigned int cursor;
  SD_task_t task;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file dot_file [trace_file]"
             "example: %s ../2clusters.xml dag.dot dag.mytrace", argv[0], argv[0]);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DOT file  and schedule tasks */
  dot = SD_dotload_with_sched(argv[2]);
  if(!dot){
    XBT_CRITICAL("The dot file with the provided scheduling is wrong, more information with the option : --log=sd_dotparse.thres:verbose");
    SD_exit();
    exit(2);
  }

  char *tracefilename;
  char *last = strrchr(argv[2], '.');
  tracefilename = bprintf("%.*s.trace", (int) (last == NULL ? strlen(argv[2]) : last - argv[2]),argv[2]);
  if (argc == 4) 
    tracefilename = xbt_strdup(argv[3]);
  

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

  XBT_INFO("------------------- Run the schedule ---------------------------");
  SD_simulate(-1);

  XBT_INFO("------------------- Produce the trace file---------------------------");
  XBT_INFO("Producing the trace of the run into %s", basename(tracefilename));
  FILE *out = fopen(tracefilename, "w");
  xbt_assert(out, "Cannot write to %s", tracefilename);
  free(tracefilename);

  xbt_dynar_foreach(dot, cursor, task) {
    int kind = SD_task_get_kind(task);
    sg_host_t *wsl = SD_task_get_workstation_list(task);
    switch (kind) {
    case SD_TASK_COMP_SEQ:
      fprintf(out, "[%f->%f] %s compute %f flops # %s\n",
          SD_task_get_start_time(task), SD_task_get_finish_time(task),
          sg_host_get_name(wsl[0]), SD_task_get_amount(task), SD_task_get_name(task));
      break;
    case SD_TASK_COMM_E2E:
      fprintf(out, "[%f -> %f] %s -> %s transfer of %.0f bytes # %s\n",
          SD_task_get_start_time(task), SD_task_get_finish_time(task),
          sg_host_get_name(wsl[0]), sg_host_get_name(wsl[1]), SD_task_get_amount(task), SD_task_get_name(task));
      break;
    default:
      xbt_die("Task %s is of unknown kind %d", SD_task_get_name(task), SD_task_get_kind(task));
    }
    SD_task_destroy(task);
  }
  fclose(out);
  xbt_dynar_free_container(&dot);

  /* exit */
  SD_exit();
  return 0;
}
