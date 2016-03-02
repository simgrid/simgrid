/* Latency tests                                                            */

/* Copyright (c) 2007, 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"

#define TASK_NUM 3

/**
 * 3 tasks send 1 byte in parallel 
 * 3 flows exceed bandwidth
 * should be 10001.5
 * because the max tcp win size is 20000
 */

int main(int argc, char **argv)
{
  double communication_amount[] = { 0.0, 1.0, 0.0, 0.0 };
  double no_cost[] = { 0.0, 0.0 };

  SD_task_t task[TASK_NUM];

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  SD_task_t root = SD_task_create("Root", NULL, 1.0);
  SD_task_schedule(root, 1, sg_host_list(), no_cost, no_cost, -1.0);

  for (int i = 0; i < TASK_NUM; i++) {
    task[i] = SD_task_create("Comm", NULL, 1.0);
    SD_task_schedule(task[i], 2, sg_host_list(), no_cost, communication_amount, -1.0);
    SD_task_dependency_add(NULL, NULL, root, task[i]);
  }

  SD_simulate(-1.0);

  printf("%g\n", SD_get_clock());
  fflush(stdout);

  for (int i = 0; i < TASK_NUM; i++) {
    SD_task_destroy(task[i]);
  }
  SD_task_destroy(root);

  SD_exit();
  return 0;
}
