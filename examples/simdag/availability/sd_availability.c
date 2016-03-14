/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_avail, "Logging specific to this SimDag example");

/* Test of dynamic availability traces
 * Scenario:
 *  - A chain of tasks: t1 -> c1 -> t2 -> c2 -> t3 -> c3 -> t4 alternatively
 *    scheduled across two workstations (Tremblay and Jupiter) connected by a
 *    single link (link1)
 *  - Make the characteristics of the resources change across time
 *      Jupiter
 *         [0.000000 -> 2.000000[ runs at half speed: 12500000
 *         [2.000000 -> end[      runs at full speed: 25000000
 *      Tremblay
 *         [0.000000 -> 1.000000[ runs at half speed: 15000000
 *         [1.000000 -> 4.000000[ runs at full speed: 25000000
 *         [4.000000 -> 6.000000[ runs at half speed: 12500000
 *         then loop back.
 *      link1
 *         [0.000000 -> 2.000000[ bandwidth = 125000000
 *         [2.000000 -> 4.000000[ bandwidth = 62500000
 *         [4.000000 -> 6.000000[ bandwidth = 31250000
 *         then loop back.
 *  - Adjust tasks' amounts to have comprehensive execution times
 *      t1: 25000000 flops, should last 2 seconds
 *      c1: 125000000 bytes, should last 1.0001 seconds
 *      t2: 25000000 flops, should last 1 second
 *      c2: 62500000 bytes, should last 1.0001 seconds
 *      t3: 25000000 flops, should last 1 second
 *      c3: 31250000 bytes, should last 1.0001 seconds
 *      t4: 25000000 flops, should last 1 second
 */

int main(int argc, char **argv)
{
  unsigned int ctr;
  SD_task_t task;
  xbt_dynar_t changed_tasks;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);
  const sg_host_t *hosts = sg_host_list();

  SD_task_t t1 = SD_task_create_comp_seq("t1", NULL, 25000000);
  SD_task_t c1 = SD_task_create_comm_e2e("c1", NULL, 125000000);
  SD_task_t t2 = SD_task_create_comp_seq("t2", NULL, 25000000);
  SD_task_t c2 = SD_task_create_comm_e2e("c2", NULL, 62500000);
  SD_task_t t3 = SD_task_create_comp_seq("t3", NULL, 25000000);
  SD_task_t c3 = SD_task_create_comm_e2e("c3", NULL, 31250000);
  SD_task_t t4 = SD_task_create_comp_seq("t4", NULL, 25000000);

  /* Add dependencies: t1->c1->t2->c2->t3 */
  SD_task_dependency_add(NULL, NULL, t1, c1);
  SD_task_dependency_add(NULL, NULL, c1, t2);
  SD_task_dependency_add(NULL, NULL, t2, c2);
  SD_task_dependency_add(NULL, NULL, c2, t3);
  SD_task_dependency_add(NULL, NULL, t3, c3);
  SD_task_dependency_add(NULL, NULL, c3, t4);

  /* Schedule tasks t1 and w3 on first host, t2 on second host */
  /* Transfers are auto-scheduled */
  SD_task_schedulel(t1, 1, hosts[0]);
  SD_task_schedulel(t2, 1, hosts[1]);
  SD_task_schedulel(t3, 1, hosts[0]);
  SD_task_schedulel(t4, 1, hosts[1]);

  /* Add some watchpoint upon task completion */
  SD_task_watch(t1, SD_DONE);
  SD_task_watch(c1, SD_DONE);
  SD_task_watch(t2, SD_DONE);
  SD_task_watch(c2, SD_DONE);
  SD_task_watch(t3, SD_DONE);
  SD_task_watch(c3, SD_DONE);
  SD_task_watch(t4, SD_DONE);

  while (!xbt_dynar_is_empty((changed_tasks = SD_simulate(-1.0)))) {
    XBT_INFO("link1: bw=%.0f, lat=%f", SD_route_get_bandwidth(hosts[0], hosts[1]),
             SD_route_get_latency(hosts[0], hosts[1]));
    XBT_INFO("Jupiter: speed=%.0f", sg_host_speed(hosts[0])* sg_host_get_available_speed(hosts[0]));
    XBT_INFO("Tremblay: speed=%.0f", sg_host_speed(hosts[1])* sg_host_get_available_speed(hosts[1]));
    xbt_dynar_foreach(changed_tasks, ctr, task) {
      XBT_INFO("Task '%s' start time: %f, finish time: %f", SD_task_get_name(task),
           SD_task_get_start_time(task), SD_task_get_finish_time(task));
      if (SD_task_get_state(task)==SD_DONE)
        SD_task_destroy(task);
    }
  }
  SD_exit();
  return 0;
}
