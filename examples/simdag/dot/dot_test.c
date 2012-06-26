/* simple test trying to load a DOT file.                                   */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include <string.h>
#include <libgen.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  xbt_dynar_t dot, changed;
  unsigned int cursor;
  SD_task_t task;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  if (argc < 3) {
    XBT_INFO("Usage: %s platform_file dot_file [trace_file]", argv[0]);
    XBT_INFO("example: %s ../2clusters.xml dag.dot dag.mytrace", argv[0]);
    exit(1);
  }
  char *tracefilename;
  if (argc == 3) {
    char *last = strrchr(argv[2], '.');

    tracefilename =
        bprintf("%.*s.trace",
                (int) (last == NULL ? strlen(argv[2]) : last - argv[2]),
                argv[2]);
  } else {
    tracefilename = xbt_strdup(argv[3]);
  }

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DOT file */
  dot = SD_dotload(argv[2]);
  if(dot == NULL){
    xbt_die("No dot load may be you have a cycle in your graph");
  }

  /* Display all the tasks */
  XBT_INFO
      ("------------------- Display all tasks of the loaded DAG ---------------------------");
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

  /* Schedule them all on the first workstation */
  XBT_INFO("------------------- Schedule tasks ---------------------------");
  const SD_workstation_t *ws_list = SD_workstation_get_list();

  int count = SD_workstation_get_number();
  xbt_dynar_foreach(dot, cursor, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ) {
      if (!strcmp(SD_task_get_name(task), "end"))
        SD_task_schedulel(task, 1, ws_list[0]);
      else
        SD_task_schedulel(task, 1, ws_list[cursor % count]);
    }
  }

  XBT_INFO
      ("------------------- Run the schedule ---------------------------");
  changed = SD_simulate(-1);
  xbt_dynar_free_container(&changed);
  XBT_INFO
      ("------------------- Produce the trace file---------------------------");
  XBT_INFO("Producing the trace of the run into %s", basename(tracefilename));
  FILE *out = fopen(tracefilename, "w");
  xbt_assert(out, "Cannot write to %s", tracefilename);
  free(tracefilename);

  xbt_dynar_foreach(dot, cursor, task) {
    int kind = SD_task_get_kind(task);
    SD_workstation_t *wsl = SD_task_get_workstation_list(task);
    switch (kind) {
    case SD_TASK_COMP_SEQ:
      fprintf(out, "[%f] %s compute %f # %s\n",
              SD_task_get_start_time(task),
              SD_workstation_get_name(wsl[0]), SD_task_get_amount(task),
              SD_task_get_name(task));
      break;
    case SD_TASK_COMM_E2E:
      fprintf(out, "[%f] %s send %s %f # %s\n",
              SD_task_get_start_time(task),
              SD_workstation_get_name(wsl[0]),
              SD_workstation_get_name(wsl[1]), SD_task_get_amount(task),
              SD_task_get_name(task));
      fprintf(out, "[%f] %s recv %s %f # %s\n",
              SD_task_get_finish_time(task),
              SD_workstation_get_name(wsl[1]),
              SD_workstation_get_name(wsl[0]), SD_task_get_amount(task),
              SD_task_get_name(task));
      break;
    default:
      xbt_die("Task %s is of unknown kind %d", SD_task_get_name(task),
              SD_task_get_kind(task));
    }
    SD_task_destroy(task);
  }
  fclose(out);

  /* exit */
  SD_exit();
  return 0;
}
