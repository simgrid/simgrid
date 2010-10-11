/* Latency tests                                                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

/**
 * bw and latency test 3
 * same intention as test 2
 * sending 2 x 1 bytes at the same time 
 * this time in opposite direction
 * 
 */

int main(int argc, char **argv)
{
  double time;
  SD_task_t root;
  SD_task_t task1;
  SD_task_t task2;
  double communication_amount1[] = { 0.0, 1.0, 0.0, 0.0 };
  double communication_amount2[] = { 0.0, 0.0, 1.0, 0.0 };
  double no_cost1[] = { 0.0 };
  double no_cost[] = { 0.0, 0.0 };

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  root = SD_task_create("Root", NULL, 1.0);
  task1 = SD_task_create("Comm 1", NULL, 1.0);
  task2 = SD_task_create("Comm 2", NULL, 1.0);

  SD_task_schedule(root, 1, SD_workstation_get_list(), no_cost1,
                   no_cost1, -1.0);
  SD_task_schedule(task1, 2, SD_workstation_get_list(), no_cost,
                   communication_amount1, -1.0);
  SD_task_schedule(task2, 2, SD_workstation_get_list(), no_cost,
                   communication_amount2, -1.0);

  SD_task_dependency_add(NULL, NULL, root, task1);
  SD_task_dependency_add(NULL, NULL, root, task2);

  SD_simulate(-1.0);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(root);
  SD_task_destroy(task1);
  SD_task_destroy(task2);

  SD_exit();

  return 0;
}
