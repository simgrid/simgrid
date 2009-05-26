
/*
 * SimDag
 * Computation tests
 * Copyright (C) 2007 
 * Sascha Hunold, Frederic Suter
 */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

int main(int argc, char **argv)
{

  double time;
  double comm_amount[] = { 0.0 };
  double comp_cost[] = { 1.0 };

  SD_task_t task;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  task = SD_task_create("seqtask", NULL, 1.0);
  SD_task_schedule(task, 1, SD_workstation_get_list(), comp_cost,
                   comm_amount, -1.0);

  SD_simulate(-1.0);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(task);

  SD_exit();

  return 0;
}
