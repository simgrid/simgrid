/* simple test trying to load a DAX file.                                   */

/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include <stdio.h>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

static int name_compare_hosts(const void *n1, const void *n2)
{
  return strcmp(sg_host_get_name(*(sg_host_t *) n1), sg_host_get_name(*(sg_host_t *) n2));
}

int main(int argc, char **argv)
{
  xbt_dynar_t dax;
  unsigned int cursor;
  SD_task_t task;

  /* SD initialization */
  SD_init(&argc, argv);

  /* Check our arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file dax_file [jedule_file]\n"
       "\tExample: %s simulacrum_7_hosts.xml Montage_25.xml Montage_25.jed", argv[0], argv[0]);

  char *last = strrchr(argv[2], '.');
  char * tracefilename = bprintf("%.*s.trace",(int) (last == NULL ? strlen(argv[2]):last - argv[2]), argv[2]);
  if (argc == 4)
    tracefilename = xbt_strdup(argv[3]);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DAX file */
  dax = SD_daxload(argv[2]);
  if (!dax){
    XBT_ERROR("A problem occurred during DAX parsing (cycle or syntax). Do not continue this test");
    free(tracefilename);
    SD_exit();
    exit(255);
  }

  /* Display all the tasks */
  XBT_INFO("------------------- Display all tasks of the loaded DAG ---------------------------");
  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_dump(task);
  }

  FILE *dotout = fopen("dax.dot", "w");
  fprintf(dotout, "digraph A {\n");
  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_dotty(task, dotout);
  }
  fprintf(dotout, "}\n");
  fclose(dotout);

  /* Schedule them all on the first host */
  XBT_INFO("------------------- Schedule tasks ---------------------------");
  const sg_host_t *ws_list = sg_host_list();
  int hosts_count = sg_host_count();
  qsort((void *) ws_list, hosts_count, sizeof(sg_host_t), name_compare_hosts);

  xbt_dynar_foreach(dax, cursor, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ) {
      if (!strcmp(SD_task_get_name(task), "end"))
        SD_task_schedulel(task, 1, ws_list[0]);
      else
        SD_task_schedulel(task, 1, ws_list[cursor % hosts_count]);
    }
  }

  XBT_INFO("------------------- Run the schedule ---------------------------");
  SD_simulate(-1);
  XBT_INFO("------------------- Produce the trace file---------------------------");
  XBT_INFO("Producing the trace of the run into %s", tracefilename);
  FILE *out = fopen(tracefilename, "w");
  xbt_assert(out, "Cannot write to %s", tracefilename);
  free(tracefilename);

  xbt_dynar_foreach(dax, cursor, task) {
    int kind = SD_task_get_kind(task);
    sg_host_t *wsl = SD_task_get_workstation_list(task);
    switch (kind) {
    case SD_TASK_COMP_SEQ:
      fprintf(out, "[%f] %s compute %f # %s\n", SD_task_get_start_time(task), sg_host_get_name(wsl[0]),
              SD_task_get_amount(task), SD_task_get_name(task));
      break;
    case SD_TASK_COMM_E2E:
      fprintf(out, "[%f] %s send %s %f # %s\n", SD_task_get_start_time(task), sg_host_get_name(wsl[0]),
              sg_host_get_name(wsl[1]), SD_task_get_amount(task), SD_task_get_name(task));
      fprintf(out, "[%f] %s recv %s %f # %s\n", SD_task_get_finish_time(task), sg_host_get_name(wsl[1]),
              sg_host_get_name(wsl[0]), SD_task_get_amount(task), SD_task_get_name(task));
      break;
    default:
      xbt_die("Task %s is of unknown kind %d", SD_task_get_name(task), SD_task_get_kind(task));
    }
    SD_task_destroy(task);
  }
  fclose(out);
  xbt_dynar_free_container(&dax);

  SD_exit();
  return 0;
}
