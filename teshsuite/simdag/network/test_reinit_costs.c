/* Computation tests                                                        */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "simdag/simdag.h"

/*
 * This test checks if the reinitialization of
 * surf works properly. 
 * 1 test: empty task, reinit, empty task
 * 2 test: comm cost task, reinit, empty task
 * 
 * output should be:
 * 0
 * 1.5
 */

static SD_task_t create_empty_cost_root()
{
  double no_cost[] = { 0.0 };
  SD_task_t root;

  root = SD_task_create("Root", NULL, 1.0);
  SD_task_schedule(root, 1, SD_workstation_get_list(), no_cost, no_cost,
                   -1.0);

  return root;
}

static void zero_cost_test(int *argc, char *argv[])
{
  double time;
  SD_task_t task;

  SD_init(argc, argv);
  SD_create_environment(argv[1]);

  task = create_empty_cost_root();
  SD_simulate(-1.0);
  SD_task_destroy(task);

  SD_application_reinit();

  task = create_empty_cost_root();
  SD_simulate(-1.0);
  SD_task_destroy(task);

  SD_simulate(-1.0);

  time = SD_get_clock();
  printf("%g\n", time);
  fflush(stdout);

  SD_exit();
}

static SD_task_t create_root_with_costs()
{
  double comp_cost[] = { 0.0, 0.0 };
  double comm_cost[] = { 0.0, 1.0, 0.0, 0.0 };
  SD_task_t root;

  root = SD_task_create("Root", NULL, 1.0);
  SD_task_schedule(root, 2, SD_workstation_get_list(), comp_cost,
                   comm_cost, -1.0);

  return root;
}

static void zero_cost_test2(int *argc, char *argv[])
{
  double time;
  SD_task_t task;

  SD_init(argc, argv);
  SD_create_environment(argv[1]);

  task = create_root_with_costs();
  SD_simulate(-1.0);
  SD_task_destroy(task);

  SD_application_reinit();

  task = create_empty_cost_root();
  SD_simulate(-1.0);
  SD_task_destroy(task);

  SD_simulate(-1.0);

  time = SD_get_clock();
  printf("%g\n", time);
  fflush(stdout);

  SD_exit();
}

int main(int argc, char **argv)
{

  zero_cost_test(&argc, argv);

  zero_cost_test2(&argc, argv);

  return 0;
}
