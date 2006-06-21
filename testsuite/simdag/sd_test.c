#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv) {
  
  /* No deployment file
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n", argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
     exit(1);
  }
  */

  if (argc < 2) {
     printf ("Usage: %s platform_file\n", argv[0]);
     printf ("example: %s msg_platform.xml\n", argv[0]);
     exit(1);
  }

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* creation of the tasks and their dependencies */
  SD_task_t task1 = SD_task_create("Task 1", NULL, 10.0);

  /* watch points */
  SD_task_watch(task1, SD_SCHEDULED);
  SD_task_watch(task1, SD_DONE);
  SD_task_unwatch(task1, SD_SCHEDULED);
  SD_task_watch(task1, SD_DONE);
  SD_task_watch(task1, SD_SCHEDULED);
  
  /* let's launch the simulation! */
  SD_simulate(100);

  SD_task_destroy(task1);
  SD_exit();
  return 0;
}
