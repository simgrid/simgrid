/* Copyright (c) 2007-2012, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic1, sd, "SimDag test basic1");

/* Basic SimDag Test 1
 * Scenario:
 *   - Create a no-op Init task
 *   - Create two communication tasks: 1GB and 1GB
 *   - Schedule them concurrently on the two hosts of the platform
 * The two communications occur simultaneously. They share the network for the
 * whole duration of the simulation.
 * Simulated time should be:
 *          1e9/(1/2*1.25e8) + 1e-4 = 16.0001 seconds
 */
int main(int argc, char **argv)
{
  /* scheduling parameters */
  double communication_amount1 = 1e9;
  double communication_amount2 = 1e9;
  double no_cost = 0.0;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  SD_task_t taskInit = SD_task_create("Init", NULL, 1.0);
  SD_task_t taskA = SD_task_create("Task Comm A", NULL, 1.0);
  SD_task_t taskB = SD_task_create("Task Comm B", NULL, 1.0);

  SD_task_dependency_add(NULL, NULL, taskInit, taskA);
  SD_task_dependency_add(NULL, NULL, taskInit, taskB);

  const sg_host_t *hosts = sg_host_list();

  SD_task_schedule(taskInit, 1, sg_host_list(), &no_cost, &no_cost, -1.0);
  SD_task_schedule(taskA, 1, &hosts[0], &no_cost, &communication_amount1, -1.0);
  SD_task_schedule(taskB, 1, &hosts[1], &no_cost, &communication_amount2, -1.0);

  /* let's launch the simulation! */
  SD_simulate(-1.0);
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskInit);

  XBT_INFO("Simulation time: %f", SD_get_clock());

  SD_exit();
  return 0;
}
