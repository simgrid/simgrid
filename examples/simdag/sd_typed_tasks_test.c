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
  SD_task_t task, seq_comp1, e2e_comm, seq_comp2;
  SD_task_t par_comp1, redist, par_comp2, par_comp3;
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

  for (i=0;i<SD_workstation_get_number();i++)
    XBT_INFO("%s runs at %f flops", SD_workstation_get_name(workstations[i]),
       SD_workstation_get_power(workstations[i]));

  /* creation of some typed tasks and their dependencies */
  seq_comp1 = SD_task_create_comp_seq("Seq. comp. 1", NULL, 1e9);
  e2e_comm = SD_task_create_comm_e2e("E2E comm.", NULL, 1e7);
  seq_comp2 = SD_task_create_comp_seq("Seq. comp 2.", NULL, 1e9);
  par_comp1 = SD_task_create_comp_par_amdahl("Par. Comp. 1", NULL, 1e9, 0.2);
  redist = SD_task_create_comm_par_mxn_1d_block("MxN redist", NULL, 1.2e8);
  par_comp2 = SD_task_create_comp_par_amdahl("Par. Comp. 2", NULL, 3e8, 0.5);

  par_comp3 = SD_task_create("Par. Comp. 3", NULL, 1e9);

  SD_task_dependency_add(NULL, NULL, seq_comp1, e2e_comm);
  SD_task_dependency_add(NULL, NULL, e2e_comm, seq_comp2);

  SD_task_dependency_add(NULL, NULL, par_comp1, redist);
  SD_task_dependency_add(NULL, NULL, redist, par_comp2);

  SD_task_schedulel(seq_comp1, 1, workstations[8]);
  SD_task_schedulel(seq_comp2, 1, workstations[9]);

  SD_task_schedulev(par_comp1, 4, workstations);
  SD_task_schedulev(par_comp2, 3, workstations);

  for (i=0;i<4;i++){
    workstation_list[i]=workstations[i+4];
    /* Apply Amdahl's law manually assuming a 20% serial part */
    computation_amount[i]=(0.2 + (1 - 0.2)/4) * SD_task_get_amount(par_comp3);
  }

  SD_task_schedule(par_comp3, 4, workstation_list,
                   computation_amount, communication_amount, -1);

  changed_tasks = SD_simulate(-1.0);
  xbt_dynar_foreach(changed_tasks, ctr, task) {
    XBT_INFO("Task '%s' start time: %f, finish time: %f",
          SD_task_get_name(task),
          SD_task_get_start_time(task), SD_task_get_finish_time(task));
    SD_task_destroy(task);
  }

  SD_exit();
  return 0;
}
