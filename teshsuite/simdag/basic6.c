/* test scheduling 2 tasks at the same time without artificial dependencies */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv)
{
  double comm_cost[] = { 0.0, 0.0, 0.0, 0.0 };
  double comp_cost[] = { 1.0 };
  SD_task_t taskA, taskB;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  taskA = SD_task_create("Task A", NULL, 1.0);
  taskB = SD_task_create("Task B", NULL, 1.0);

  SD_task_schedule(taskA, 1, SD_workstation_get_list(), comp_cost, comm_cost,
                   -1.0);
  SD_task_schedule(taskB, 1, SD_workstation_get_list(), comp_cost, comm_cost,
                   -1.0);

  SD_simulate(-1.0);

  SD_exit();
  return 0;
}
