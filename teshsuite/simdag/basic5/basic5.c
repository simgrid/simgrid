/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic5, sd, "SimDag test basic5");

/* Basic SimDag Test 5
 * Scenario:
 *   - Create a no-op Init task
 *   - Create two tasks: send 100kB and compute 10Mflops
 *   - Schedule them concurrently
 * The two tasks should overlap smoothly as they use different resources.
 * Simulated time should be:
 *          MAX(1e5/(1.25e8), 1e7/4e9) = MAX(.0009, .0025) = 0.0025 seconds
 */
int main(int argc, char **argv)
{
  /* scheduling parameters */
  double no_cost[] = { 0., 0., 0., 0. };
  double amount[] = { 0., 100000., 0., 0. };
  double comput[] = { 10000000. };

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  SD_task_t taskInit = SD_task_create("Task Init", NULL, 1.0);
  SD_task_t taskA = SD_task_create("Task A", NULL, 1.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 1.0);

  SD_task_dependency_add(taskInit, taskA);
  SD_task_dependency_add(taskInit, taskB);

  sg_host_t *hosts = sg_host_list();
  SD_task_schedule(taskInit, 1, hosts, no_cost, no_cost, -1.0);
  SD_task_schedule(taskA, 2, hosts, no_cost, amount, -1.0);
  SD_task_schedule(taskB, 1, hosts, comput, no_cost, -1.0);
  xbt_free(hosts);

  /* let's launch the simulation! */
  SD_simulate(-1.0);
  SD_task_destroy(taskInit);
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);

  XBT_INFO("Simulation time: %f", SD_get_clock());

  return 0;
}
