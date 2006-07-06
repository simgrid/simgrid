#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
			     "Logging specific to this SimDag example");

int main(int argc, char **argv) {
  int i;
  
  /* initialisation of SD */
  SD_init(&argc, argv);

/*   xbt_log_control_set("sd_test.thres=debug"); */
/*   xbt_log_control_set("sd.thres=debug"); */
  
  if (argc < 2) {
     INFO1("Usage: %s platform_file", argv[0]);
     INFO1("example: %s sd_platform.xml", argv[0]);
     exit(1);
  }

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* test the estimation functions (use small_platform.xml) */
  SD_workstation_t w1 = SD_workstation_get_by_name("Jupiter");
  SD_workstation_t w2 = SD_workstation_get_by_name("Ginette");
  const char *name1 = SD_workstation_get_name(w1);
  const char *name2 = SD_workstation_get_name(w2);
  const double computation_amount1 = 2000000;
  const double computation_amount2 = 1000000;
  const double communication_amount12 = 2000000;
  const double communication_amount21 = 3000000;
  INFO3("Computation time for %f flops on %s: %f", computation_amount1, name1,
	SD_workstation_get_computation_time(w1, computation_amount1));
  INFO3("Computation time for %f flops on %s: %f", computation_amount2, name2,
	SD_workstation_get_computation_time(w2, computation_amount2));

  INFO2("Route between %s and %s:", name1, name2);
  SD_link_t *route = SD_workstation_route_get_list(w1, w2);
  int route_size = SD_workstation_route_get_size(w1, w2);
  for (i = 0; i < route_size; i++) {
    INFO3("\tLink %s: latency = %f, bandwidth = %f", SD_link_get_name(route[i]),
	  SD_link_get_current_latency(route[i]), SD_link_get_current_bandwidth(route[i]));
  }
  INFO2("Route latency = %f, route bandwidth = %f", SD_workstation_route_get_latency(w1, w2),
	SD_workstation_route_get_bandwidth(w1, w2));
  INFO4("Communication time for %f bytes between %s and %s: %f", communication_amount12, name1, name2,
	SD_workstation_route_get_communication_time(w1, w2, communication_amount12));
  INFO4("Communication time for %f bytes between %s and %s: %f", communication_amount21, name2, name1,
	SD_workstation_route_get_communication_time(w2, w1, communication_amount21));
  xbt_free(route);

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);
  
  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  /*  SD_task_dependency_add(NULL, NULL, taskA, taskD); /\* deadlock *\/ */

  /* scheduling parameters */

  const int workstation_number = 2;
  /*  const SD_workstation_t *workstation_list = SD_workstation_get_list();*/
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

  SD_task_watch(taskC, SD_DONE);

  SD_task_t *changed_tasks;

  changed_tasks = SD_simulate(0.001);
  
  while (changed_tasks[0] != NULL) {
    INFO0("Tasks whose state has changed:");
    i = 0;
    while(changed_tasks[i] != NULL) {
      switch (SD_task_get_state(changed_tasks[i])) {
      case SD_SCHEDULED:
	INFO1("%s is scheduled.", SD_task_get_name(changed_tasks[i]));
	break;
      case SD_READY:
	INFO1("%s is ready.", SD_task_get_name(changed_tasks[i]));
	break;
      case SD_RUNNING:
	INFO1("%s is running.", SD_task_get_name(changed_tasks[i]));
	break;
      case SD_DONE:
	INFO1("%s is done.", SD_task_get_name(changed_tasks[i]));
	break;
      case SD_FAILED:
	INFO1("%s is failed.", SD_task_get_name(changed_tasks[i]));
	break;
      default:
	INFO1("Unknown status for %s", SD_task_get_name(changed_tasks[i]));
	break;
      }
      i++;
    }
    xbt_free(changed_tasks);
    changed_tasks = SD_simulate(100);
  }

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
