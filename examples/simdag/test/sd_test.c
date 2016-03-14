/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  unsigned int ctr;
  SD_task_t checkB, checkD;
  xbt_dynar_t changed_tasks;
  xbt_ex_t ex;
  const int host_count = 2;
  sg_host_t host_list[2];
  double computation_amount[2];
  double communication_amount[4] = { 0 };
  double rate = -1.0;

  /* initialization of SD */
  SD_init(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\nExample: %s two_clusters.xml", argv[0], argv[0]);
  SD_create_environment(argv[1]);

  /* test the estimation functions */
  const sg_host_t *hosts = sg_host_list();
  sg_host_t h1 = hosts[0];
  sg_host_t h2 = hosts[1];
  const char *name1 = sg_host_get_name(h1);
  const char *name2 = sg_host_get_name(h2);
  double comp_amount1 = 2000000;
  double comp_amount2 = 1000000;
  double comm_amount12 = 2000000;
  double comm_amount21 = 3000000;
  XBT_INFO("Computation time for %f flops on %s: %f", comp_amount1, name1, comp_amount1/sg_host_speed(h1));
  XBT_INFO("Computation time for %f flops on %s: %f", comp_amount2, name2, comp_amount2/sg_host_speed(h2));

  XBT_INFO("Route between %s and %s:", name1, name2);
  const SD_link_t *route = SD_route_get_list(h1, h2);
  int route_size = SD_route_get_size(h1, h2);
  for (int i = 0; i < route_size; i++) {
    XBT_INFO("   Link %s: latency = %f, bandwidth = %f", sg_link_name(route[i]), sg_link_latency(route[i]),
             sg_link_bandwidth(route[i]));
  }
  XBT_INFO("Route latency = %f, route bandwidth = %f", SD_route_get_latency(h1, h2), SD_route_get_bandwidth(h1, h2));
  XBT_INFO("Communication time for %f bytes between %s and %s: %f", comm_amount12, name1, name2,
        SD_route_get_latency(h1, h2) + comm_amount12 / SD_route_get_bandwidth(h1, h2));
  XBT_INFO("Communication time for %f bytes between %s and %s: %f", comm_amount21, name2, name1,
        SD_route_get_latency(h2, h1) + comm_amount21 / SD_route_get_bandwidth(h2, h1));

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);

  /* try to attach and retrieve user data to a task */
  SD_task_set_data(taskA, (void*) &comp_amount1);
  if (comp_amount1 != (*((double*) SD_task_get_data(taskA))))
      XBT_ERROR("User data was corrupted by a simple set/get");

  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  SD_task_dependency_add(NULL, NULL, taskB, taskC);

  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskA);   /* shouldn't work and must raise an exception */
    xbt_die("Hey, I can add a dependency between Task A and Task A!");
  }
  CATCH(ex) {
    if (ex.category != arg_error)
      RETHROW;                  /* this is a serious error */
    xbt_ex_free(ex);
  }

  TRY {
    SD_task_dependency_add(NULL, NULL, taskB, taskA);   /* shouldn't work and must raise an exception */
    xbt_die("Oh oh, I can add an already existing dependency!");
  }
  CATCH(ex) {
    if (ex.category != arg_error)
      RETHROW;
    xbt_ex_free(ex);
  }

  TRY {
    SD_task_dependency_remove(taskA, taskC);    /* shouldn't work and must raise an exception */
    xbt_die("Dude, I can remove an unknown dependency!");
  }
  CATCH(ex) {
    if (ex.category != arg_error)
      RETHROW;
    xbt_ex_free(ex);
  }

  TRY {
    SD_task_dependency_remove(taskC, taskC);    /* shouldn't work and must raise an exception */
    xbt_die("Wow, I can remove a dependency between Task C and itself!");
  }
  CATCH(ex) {
    if (ex.category != arg_error)
      RETHROW;
    xbt_ex_free(ex);
  }

  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskD, SD_DONE);
  SD_task_watch(taskB, SD_DONE);
  SD_task_unwatch(taskD, SD_DONE);

  /* scheduling parameters */
  host_list[0] = h1;
  host_list[1] = h2;
  computation_amount[0] = comp_amount1;
  computation_amount[1] = comp_amount2;

  communication_amount[1] = comm_amount12;
  communication_amount[2] = comm_amount21;

  /* estimated time */
  SD_task_t task = taskD;
  XBT_INFO("Estimated time for '%s': %f", SD_task_get_name(task), SD_task_get_execution_time(task, host_count,
           host_list, computation_amount, communication_amount));

  SD_task_schedule(taskA, host_count, host_list, computation_amount, communication_amount, rate);
  SD_task_schedule(taskB, host_count, host_list, computation_amount, communication_amount, rate);
  SD_task_schedule(taskC, host_count, host_list, computation_amount, communication_amount, rate);
  SD_task_schedule(taskD, host_count, host_list, computation_amount, communication_amount, rate);

  changed_tasks = SD_simulate(-1.0);
  xbt_dynar_foreach(changed_tasks, ctr, task) {
    XBT_INFO("Task '%s' start time: %f, finish time: %f", SD_task_get_name(task),
          SD_task_get_start_time(task), SD_task_get_finish_time(task));
  }

  xbt_dynar_get_cpy(changed_tasks, 0, &checkD);
  xbt_dynar_get_cpy(changed_tasks, 1, &checkB);

  xbt_assert(checkD == taskD && checkB == taskB, "Unexpected simulation results");

  XBT_DEBUG("Destroying tasks...");
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  XBT_DEBUG("Tasks destroyed. Exiting SimDag...");
  SD_exit();
  return 0;
}
