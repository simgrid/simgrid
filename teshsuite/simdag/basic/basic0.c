/* Copyright (c) 2007-2012, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic0, sd, "SimDag test basic0");

/* Basic SimDag Test 0
 * Scenario:
 *   - Create a no-op Init task
 *   - Create two communication tasks: 100MB and 1B
 *   - Schedule them concurrently on the two hosts of the platform
 * The two communications occur simultaneously but one is so short that it has no impact on the other.
 * Simulated time should be:
 *          1e8/1.25e8 + 1e-4 = 0.8001 seconds
 * This corresponds to paying latency once and having the full bandwidth for the big message.
 */
int main(int argc, char **argv)
{
  /* scheduling parameters */
  double communication_amount1[] = { 0, 1e8, 0, 0 };
  double communication_amount2[] = { 0, 1, 0, 0 };
  const double no_cost[] = { 0.0, 0.0 };

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  SD_task_t taskInit = SD_task_create("Init", NULL, 1.0);
  SD_task_t taskA = SD_task_create("Task Comm 1", NULL, 1.0);
  SD_task_t taskB = SD_task_create("Task Comm 2", NULL, 1.0);

  SD_task_dependency_add(NULL, NULL, taskInit, taskA);
  SD_task_dependency_add(NULL, NULL, taskInit, taskB);

  SD_task_schedule(taskInit, 1, sg_host_list(), no_cost, no_cost, -1.0);
  SD_task_schedule(taskA, 2, sg_host_list(), no_cost, communication_amount1, -1.0);
  SD_task_schedule(taskB, 2, sg_host_list(), no_cost, communication_amount2, -1.0);

  /* let's launch the simulation! */
  SD_simulate(-1.0);
  SD_task_destroy(taskInit);
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);

  XBT_INFO("Simulation time: %f", SD_get_clock());

  SD_exit();
  return 0;
}
