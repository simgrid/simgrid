/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  int i;
  const char *platform_file;

  const SD_workstation_t *workstations;
  SD_workstation_t w1;
  SD_workstation_t w2;
  const char *name1 = NULL;
  const char *name2 = NULL;
  const double computation_amount1 = 2000000;
  const double computation_amount2 = 1000000;
  const double communication_amount12 = 2000000;
  const double communication_amount21 = 3000000;
  const SD_link_t *route;
  int route_size;
  SD_task_t taskA;
  SD_task_t taskB, checkB;
  SD_task_t taskC, checkC;
  SD_task_t taskD, checkD;
  xbt_ex_t ex;

  /* initialisation of SD */
  SD_init(&argc, argv);

  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */

  platform_file = argv[1];

  SD_create_environment(platform_file);

  /* test the estimation functions (use small_platform.xml) */
  workstations = SD_workstation_get_list();


  for (i = 0; i < SD_workstation_get_number(); i++) {
    INFO1("name : %s", SD_workstation_get_name(workstations[i]));
  }

  w1 = workstations[0];
  w2 = workstations[1];
  name1 = SD_workstation_get_name(w1);
  name2 = SD_workstation_get_name(w2);

  route = SD_route_get_list(w1, w2);

  route_size = SD_route_get_size(w1, w2);

  taskA = SD_task_create("Task A", NULL, 10.0);
  taskB = SD_task_create("Task B", NULL, 40.0);
  taskC = SD_task_create("Task C", NULL, 30.0);
  taskD = SD_task_create("Task D", NULL, 60.0);

  INFO3("Computation time for %f flops on %s: %f", computation_amount1,
        name1, SD_workstation_get_computation_time(w1,
                                                   computation_amount1));
  INFO3("Computation time for %f flops on %s: %f", computation_amount2,
        name2, SD_workstation_get_computation_time(w2,
                                                   computation_amount2));

  INFO2("Route between %s and %s:", name1, name2);
  for (i = 0; i < route_size; i++) {
    INFO3("\tLink %s: latency = %f, bandwidth = %f",
          SD_link_get_name(route[i]),
          SD_link_get_current_latency(route[i]),
          SD_link_get_current_bandwidth(route[i]));
  }
  INFO2("Route latency = %f, route bandwidth = %f",
        SD_route_get_current_latency(w1, w2),
        SD_route_get_current_bandwidth(w1, w2));
  INFO4("Communication time for %f bytes between %s and %s: %f",
        communication_amount12, name1, name2,
        SD_route_get_communication_time(w1, w2, communication_amount12));
  INFO4("Communication time for %f bytes between %s and %s: %f",
        communication_amount21, name2, name1,
        SD_route_get_communication_time(w2, w1, communication_amount21));

  /* creation of the tasks and their dependencies */

  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  /*  SD_task_dependency_add(NULL, NULL, taskA, taskD); /\* deadlock */

  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskA);   /* shouldn't work and must raise an exception */
    xbt_assert0(0,
                "Hey, I can add a dependency between Task A and Task A!");
  }
  CATCH(ex) {
  }

  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskB);   /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Oh oh, I can add an already existing dependency!");
  }
  CATCH(ex) {
  }

  SD_task_dependency_remove(taskA, taskB);

  TRY {
    SD_task_dependency_remove(taskC, taskA);    /* shouldn't work and must raise an exception */
    xbt_assert0(0, "Dude, I can remove an unknown dependency!");
  }
  CATCH(ex) {
  }

  TRY {
    SD_task_dependency_remove(taskC, taskC);    /* shouldn't work and must raise an exception */
    xbt_assert0(0,
                "Wow, I can remove a dependency between Task C and itself!");
  }
  CATCH(ex) {
  }


  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskD, SD_DONE);
  SD_task_watch(taskB, SD_DONE);
  SD_task_unwatch(taskD, SD_DONE);


  /* scheduling parameters */
  {
    const int workstation_number = 2;
    const SD_workstation_t workstation_list[] = { w1, w2 };
    double computation_amount[] =
        { computation_amount1, computation_amount2 };
    double communication_amount[] = {
      0, communication_amount12,
      communication_amount21, 0
    };
    xbt_dynar_t changed_tasks;
    double rate = -1.0;

    /* estimated time */
    SD_task_t task = taskD;
    INFO2("Estimated time for '%s': %f", SD_task_get_name(task),
          SD_task_get_execution_time(task, workstation_number,
                                     workstation_list, computation_amount,
                                     communication_amount));

    /* let's launch the simulation! */

    SD_task_schedule(taskA, workstation_number, workstation_list,
                     computation_amount, communication_amount, rate);
    SD_task_schedule(taskB, workstation_number, workstation_list,
                     computation_amount, communication_amount, rate);
    SD_task_schedule(taskC, workstation_number, workstation_list,
                     computation_amount, communication_amount, rate);
    SD_task_schedule(taskD, workstation_number, workstation_list,
                     computation_amount, communication_amount, rate);

    changed_tasks = SD_simulate(-1.0);

    xbt_dynar_get_cpy(changed_tasks, 0, &checkD);
    xbt_dynar_get_cpy(changed_tasks, 1, &checkC);
    xbt_dynar_get_cpy(changed_tasks, 2, &checkB);

    xbt_assert0(checkD == taskD &&
                checkC == taskC &&
                checkB == taskB, "Unexpected simulation results");

    xbt_dynar_free_container(&changed_tasks);
  }
  DEBUG0("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  DEBUG0("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}
