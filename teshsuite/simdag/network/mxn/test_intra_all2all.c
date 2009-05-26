
/*
 * SimDag
 * Latency tests
 * Copyright (C) 2007 
 * Sascha Hunold, Frederic Suter
 */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

/*
 * intra communication test
 * All2All
 * 
 * send 1 byte from all to all
 * + 2 secs latency
 * should be 8  (platform_4p_1switch.xml)
 * 
 */

int main(int argc, char **argv)
{
  double time;
  SD_task_t task;

  double communication_amount[] = { 0.0, 1.0, 1.0, 1.0,
    1.0, 0.0, 1.0, 1.0,
    1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 0.0,
  };

  double no_cost[] = { 0.0, 0.0, 0.0, 0.0 };


  /***************************************/

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  task = SD_task_create("All2all task", NULL, 1.0);

  SD_task_schedule(task, 4, SD_workstation_get_list(), no_cost,
                   communication_amount, -1.0);

  SD_simulate(-1.0);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(task);

  SD_exit();

  return 0;
}
