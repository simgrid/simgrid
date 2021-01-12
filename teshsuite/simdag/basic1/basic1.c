/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/str.h"

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

  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  /* scheduling parameters */
  double communication_amount1 = xbt_str_parse_double(argv[2], "Invalid communication size: %s");
  double communication_amount2 = xbt_str_parse_double(argv[3], "Invalid communication size: %s");

  /* creation of the tasks and their dependencies */
  SD_task_t taskInit = SD_task_create("Init", NULL, 1.0);
  SD_task_t taskA = SD_task_create("Task Comm A", NULL, 1.0);
  SD_task_t taskB = SD_task_create("Task Comm B", NULL, 1.0);

  SD_task_dependency_add(taskInit, taskA);
  SD_task_dependency_add(taskInit, taskB);

  sg_host_t *hosts = sg_host_list();
  SD_task_schedule(taskInit, 1, hosts, SD_SCHED_NO_COST, SD_SCHED_NO_COST, -1.0);
  SD_task_schedule(taskA, 1, &hosts[0], SD_SCHED_NO_COST, &communication_amount1, -1.0);
  SD_task_schedule(taskB, 1, &hosts[1], SD_SCHED_NO_COST, &communication_amount2, -1.0);
  xbt_free(hosts);

  /* let's launch the simulation! */
  SD_simulate(-1.0);
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskInit);

  XBT_INFO("Simulation time: %f", SD_get_clock());

  return 0;
}
