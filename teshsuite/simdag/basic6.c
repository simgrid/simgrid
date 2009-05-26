#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

/*
 * test: scheduling 2 tasks at the same time
 * without artificial dependecies
 * 
 * author: sahu
 */

int main(int argc, char **argv)
{
  double comm_cost[] = { 0.0, 0.0, 0.0, 0.0 };
  double comp_cost[] = { 1.0 };
  SD_task_t taskA, taskB;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  taskA = SD_task_create("Task A", NULL, 1.0);
  taskB = SD_task_create("Task B", NULL, 1.0);

  SD_task_schedule(taskA, 1, SD_workstation_get_list(), comp_cost, comm_cost,
                   -1.0);
  SD_task_schedule(taskB, 1, SD_workstation_get_list(), comp_cost, comm_cost,
                   -1.0);

  SD_simulate(-1.0);

  SD_exit();
  return 0;
}
