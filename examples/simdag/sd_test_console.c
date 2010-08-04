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
  unsigned int ctr;
  const char *platform_script;
  const SD_workstation_t *workstations;
  const char *name1;
  const char *name2;
  double computation_amount1;
  double computation_amount2;
  double communication_amount12;
  double communication_amount21;
  const SD_link_t *route;
  int route_size;
  SD_task_t task, taskA, taskB, taskC, taskD, checkB, checkD;
  xbt_dynar_t changed_tasks;
  xbt_ex_t ex;
  const int workstation_number = 2;
  SD_workstation_t workstation_list[2];
  double computation_amount[2];
  double communication_amount[4] = { 0 };
  double rate = -1.0;
  SD_workstation_t w1, w2;

  /* initialization of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    INFO1("Usage: %s script_file", argv[0]);
    INFO1("example: %s sd_platform_script.lua", argv[0]);
    exit(1);
  }

  /* loading the environment */
  platform_script = argv[1];
  SD_load_environment_script(platform_script);

  /* test the estimation functions */
  workstations = SD_workstation_get_list();
  w1 = workstations[0];
  w2 = workstations[1];
  SD_workstation_set_access_mode(w2, SD_WORKSTATION_SEQUENTIAL_ACCESS);
  name1 = SD_workstation_get_name(w1);
  name2 = SD_workstation_get_name(w2);
  computation_amount1 = 2000000;
  computation_amount2 = 1000000;
  communication_amount12 = 2000000;
  communication_amount21 = 3000000;
  INFO3("Computation time for %f flops on %s: %f", computation_amount1, name1,
        SD_workstation_get_computation_time(w1, computation_amount1));
  INFO3("Computation time for %f flops on %s: %f", computation_amount2, name2,
        SD_workstation_get_computation_time(w2, computation_amount2));

  INFO2("Route between %s and %s:", name1, name2);
  route = SD_route_get_list(w1, w2);
  route_size = SD_route_get_size(w1, w2);
  for (i = 0; i < route_size; i++) {
    INFO3("   Link %s: latency = %f, bandwidth = %f",
          SD_link_get_name(route[i]), SD_link_get_current_latency(route[i]),
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
  taskA = SD_task_create("Task A", NULL, 10.0);
  taskB = SD_task_create("Task B", NULL, 40.0);
  taskC = SD_task_create("Task C", NULL, 30.0);
  taskD = SD_task_create("Task D", NULL, 60.0);


  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  /*  SD_task_dependency_add(NULL, NULL, taskA, taskD); /\* deadlock */



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
  workstation_list[0] = w1;
  workstation_list[1] = w2;
  computation_amount[0] = computation_amount1;
  computation_amount[1] = computation_amount2;

  communication_amount[1] = communication_amount12;
  communication_amount[2] = communication_amount21;

  /* estimated time */
  task = taskD;
  INFO2("Estimated time for '%s': %f", SD_task_get_name(task),
        SD_task_get_execution_time(task, workstation_number, workstation_list,
                                   computation_amount, communication_amount));

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
  xbt_dynar_foreach(changed_tasks, ctr, task){
		  INFO3("Task '%s' start time: %f, finish time: %f",
          SD_task_get_name(task),
          SD_task_get_start_time(task),
          SD_task_get_finish_time(task));
  }

  xbt_dynar_get_cpy(changed_tasks, 0, &checkD);
  xbt_dynar_get_cpy(changed_tasks, 1, &checkB);

  xbt_assert0(checkD == taskD &&
              checkB == taskB, "Unexpected simulation results");

  xbt_dynar_free_container(&changed_tasks);

  DEBUG0("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  DEBUG0("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}
