/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

int main(int argc, char **argv)
{

  double time;
  double comm_amount[] = { 0.0, 0.0, 0.0, 0.0 };
  double comp_cost[] = { 1.0, 1.0 };

  SD_task_t task;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  task = SD_task_create("partask", NULL, 1.0);
  SD_task_schedule(task, 2, SD_workstation_get_list(), comp_cost,
                   comm_amount, -1.0);

  SD_simulate(-1.0);

  time = SD_get_clock();

  printf("%g\n", time);
  fflush(stdout);

  SD_task_destroy(task);

  SD_exit();

  return 0;
}
