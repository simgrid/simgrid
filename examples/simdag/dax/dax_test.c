/* simple test trying to load a DAX file.                                   */

/* Copyright (c) 2009 Da SimGrid Team. All rights reserved.                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Logging specific to this SimDag example");


int main(int argc, char **argv) {
  xbt_dynar_t dax;
  unsigned int cursor;
  SD_task_t task;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  if (argc < 3) {
    INFO1("Usage: %s platform_file dax_file", argv[0]);
    INFO1("example: %s ../sd_platform.xml Montage_50.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DAX file */
  dax=SD_daxload(argv[2]);

  /* Display all the tasks */
  INFO0("------------------- Display all tasks of the loaded DAG ---------------------------");
  xbt_dynar_foreach(dax,cursor,task) {
    SD_task_dump(task);
  }

  FILE *out = fopen("dax.dot","w");
  fprintf(out,"digraph A {\n");
  xbt_dynar_foreach(dax,cursor,task) {
    SD_task_dotty(task,out);
  }
  fprintf(out,"}\n");
  fclose(out);

  /* Schedule them all on the first workstation */
  INFO0("------------------- Schedule tasks ---------------------------");
  const SD_workstation_t *ws_list =  SD_workstation_get_list();
  xbt_dynar_foreach(dax,cursor,task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ)
      SD_task_schedulel(task,1,ws_list[0]);
  }

  INFO0("------------------- Run the schedule ---------------------------");
  SD_simulate(-1);
  INFO0("------------------- Produce the trace file---------------------------");
  xbt_dynar_foreach(dax,cursor,task) {
    int kind = SD_task_get_kind(task);
    SD_workstation_t *wsl = SD_task_get_workstation_list(task);
    switch (kind) {
    case SD_TASK_COMP_SEQ:
      INFO4("[%f] %s compute %f # %s",SD_task_get_start_time(task),
          SD_workstation_get_name(wsl[0]),SD_task_get_amount(task),
          SD_task_get_name(task));
      break;
    case SD_TASK_COMM_E2E:
      INFO5("[%f] %s send %s %f # %s",SD_task_get_start_time(task),
          SD_workstation_get_name(wsl[0]),SD_workstation_get_name(wsl[1]),
          SD_task_get_amount(task), SD_task_get_name(task));
      INFO5("[%f] %s recv %s %f # %s",SD_task_get_start_time(task),
          SD_workstation_get_name(wsl[1]),SD_workstation_get_name(wsl[0]),
          SD_task_get_amount(task), SD_task_get_name(task));
      break;
    default:
      xbt_die(bprintf("Task %s is of unknown kind %d",SD_task_get_name(task),SD_task_get_kind(task)));
    }
  }
  /* exit */
  SD_exit();
  return 1;
}
