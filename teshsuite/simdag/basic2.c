#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv)
{

  SD_task_t taskInit;
  SD_task_t taskA;
  SD_task_t taskB;

  const SD_workstation_t *workstation;

  double communication_amount1 = 1000000000;
  double communication_amount2 = 100000000;
  double no_cost = 0.0;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* creation of the tasks and their dependencies */
  taskInit = SD_task_create("Init", NULL, 1.0);
  taskA = SD_task_create("Task Comm A", NULL, 1.0);
  taskB = SD_task_create("Task Comm B", NULL, 1.0);


  /* scheduling parameters */

  workstation = SD_workstation_get_list();

  /* let's launch the simulation! */
  SD_task_schedule(taskInit, 1, SD_workstation_get_list(), &no_cost, &no_cost,
                   -1.0);
  SD_task_schedule(taskA, 1, &workstation[0], &no_cost,
                   &communication_amount1, -1.0);
  SD_task_schedule(taskB, 1, &workstation[1], &no_cost,
                   &communication_amount2, -1.0);

  SD_task_dependency_add(NULL, NULL, taskInit, taskA);
  SD_task_dependency_add(NULL, NULL, taskInit, taskB);

  SD_simulate(-1.0);

  SD_exit();
  return 0;
}
