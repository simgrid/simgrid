/*
   See examples/platforms/metaxml.xml and examples/platforms/metaxml_platform.xml files for examples on how to use the cluster, foreach, set, route:multi, trace and trace:connect tags
*/
#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/time.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
			     "Logging specific to this SimDag example");

int main(int argc, char **argv) {
  /* initialisation of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  const char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* test the estimation functions */
  const SD_workstation_t *workstations = SD_workstation_get_list();
  int ws_nr = SD_workstation_get_number();

  SD_workstation_t w1 = NULL;
  SD_workstation_t w2 = NULL;
  const char *name1, *name2;
  /* Show routes between all workstation */
  int i,j,k;
  for (i=0; i<ws_nr; i++){
    for (j=0;j<ws_nr; j++){
      w1 = workstations[i];
      w2 = workstations[j];
      name1 = SD_workstation_get_name(w1);
      name2 = SD_workstation_get_name(w2);
      INFO2("Route between %s and %s:", name1, name2);   
      const SD_link_t *route = SD_route_get_list(w1, w2);
      int route_size = SD_route_get_size(w1, w2);
      for (k = 0; k < route_size; k++) {
        INFO3("\tLink %s: latency = %f, bandwidth = %f", SD_link_get_name(route[k]),
	  SD_link_get_current_latency(route[k]), SD_link_get_current_bandwidth(route[k]));
      }
    }
  }

  /* Simulate */

  w1 = workstations[0];
  w2 = workstations[1];
  SD_workstation_set_access_mode(w2, SD_WORKSTATION_SEQUENTIAL_ACCESS);
  name1 = SD_workstation_get_name(w1);
  name2 = SD_workstation_get_name(w2);

  const double computation_amount1 = 2000000;
  const double computation_amount2 = 1000000;
  const double communication_amount12 = 2000000;
  const double communication_amount21 = 3000000;
  INFO3("Computation time for %f flops on %s: %f", computation_amount1, name1,
	SD_workstation_get_computation_time(w1, computation_amount1));
  INFO3("Computation time for %f flops on %s: %f", computation_amount2, name2,
	SD_workstation_get_computation_time(w2, computation_amount2));

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);
  

  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);

  xbt_ex_t ex;

  TRY {
    SD_task_dependency_add(NULL, NULL, taskA, taskA); /* shouldn't work and must raise an exception */
    xbt_die("Hey, I can add a dependency between Task A and Task A!");
  }
  CATCH (ex) {
    if (ex.category != arg_error)

      RETHROW; /* this is a serious error */
    xbt_ex_free(ex);
  }
  
  TRY {
    SD_task_dependency_add(NULL, NULL, taskB, taskA); /* shouldn't work and must raise an exception */
    xbt_die("Oh oh, I can add an already existing dependency!");
  }
  CATCH (ex) {
    if (ex.category != arg_error)
      RETHROW;
    xbt_ex_free(ex);
  }

  TRY {
    SD_task_dependency_remove(taskA, taskC); /* shouldn't work and must raise an exception */
    xbt_die("Dude, I can remove an unknown dependency!");
  }
  CATCH (ex) {
    if (ex.category != arg_error)
      RETHROW;
    xbt_ex_free(ex);
  }

  TRY {
    SD_task_dependency_remove(taskC, taskC); /* shouldn't work and must raise an exception */
    xbt_die("Wow, I can remove a dependency between Task C and itself!");
  }
  CATCH (ex) {
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

  const int workstation_number = 2;
  const SD_workstation_t workstation_list[] = {w1, w2};
  double computation_amount[] = {computation_amount1, computation_amount2};
  double communication_amount[] =
    {
      0, communication_amount12,
      communication_amount21, 0
    };
  double rate = -1.0;

  /* estimated time */
  SD_task_t task = taskD;
  INFO2("Estimated time for '%s': %f", SD_task_get_name(task),
	SD_task_get_execution_time(task, workstation_number, workstation_list,
				   computation_amount, communication_amount, rate));

  /* let's launch the simulation! */

  SD_task_schedule(taskA, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskB, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskC, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskD, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);

  SD_task_t *changed_tasks;

  changed_tasks = SD_simulate(-1.0);
  for (i = 0; changed_tasks[i] != NULL; i++) {
    INFO3("Task '%s' start time: %f, finish time: %f",
	  SD_task_get_name(changed_tasks[i]),
	  SD_task_get_start_time(changed_tasks[i]),
	  SD_task_get_finish_time(changed_tasks[i]));
  }
  
  xbt_assert0(changed_tasks[0] == taskD &&
	      changed_tasks[1] == taskB &&
	      changed_tasks[2] == NULL,
	      "Unexpected simulation results");

  xbt_free(changed_tasks);

  DEBUG0("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  DEBUG0("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}

