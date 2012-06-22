/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic2, sd, "SimDag test basic2");

int main(int argc, char **argv)
{

  SD_task_t taskInit;
  SD_task_t taskA, taskB, taskC, taskD;
  xbt_dynar_t ret;

  const SD_workstation_t *workstation;

  double communication_amount1 = 1000000000;
  double communication_amount2 = 100000000;
  double no_cost = 0.0;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  taskInit = SD_task_create("Init", NULL, 1.0);
  taskA = SD_task_create("Task A", NULL, 1.0);
  taskB = SD_task_create("Task B", NULL, 1.0);
  taskC = SD_task_create("Task C", NULL, 1.0);
  taskD = SD_task_create("Task D", NULL, 1.0);


  /* scheduling parameters */

  workstation = SD_workstation_get_list();

  SD_task_dependency_add(NULL, NULL, taskInit, taskA);
  SD_task_dependency_add(NULL, NULL, taskInit, taskB);
  SD_task_dependency_add(NULL, NULL, taskC, taskD);

  /* let's launch the simulation! */
  SD_task_schedule(taskInit, 1, SD_workstation_get_list(), &no_cost,
                   &no_cost, -1.0);
  SD_task_schedule(taskA, 1, &workstation[0], &no_cost,
                     &communication_amount1, -1.0);
  SD_task_schedule(taskD, 1, &workstation[0], &no_cost,
                     &communication_amount1, -1.0);


  ret = SD_simulate(-1.);
  xbt_dynar_free(&ret);
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);
  SD_task_destroy(taskInit);

  XBT_INFO("Simulation time: %f", SD_get_clock());

  SD_exit();
  return 0;
}
