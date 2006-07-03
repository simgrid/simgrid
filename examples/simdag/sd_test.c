#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test,
			     "Logging specific to this SimDag example");

int main(int argc, char **argv) {
  
  /* initialisation of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug");*/

  if (argc < 2) {
     INFO1("Usage: %s platform_file", argv[0]);
     INFO1("example: %s sd_platform.xml", argv[0]);
     exit(1);
  }

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);
  
  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);
  /*  SD_task_dependency_add(NULL, NULL, taskA, taskD); /\* deadlock */

  /* let's launch the simulation! */

  int workstation_number = 2;
  SD_workstation_t *workstation_list = SD_workstation_get_list();
  double computation_amount[] = {100, 200};
  double communication_amount[] =
    {
      0, 30,
      20, 0
    };
  double rate = 1;

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
  int i;

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
    free(changed_tasks);
    changed_tasks = SD_simulate(100);
  }

  free(changed_tasks);

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);
  SD_exit();
  return 0;
}
