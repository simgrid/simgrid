/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_fail,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  SD_task_t task;
  double computation_amount[1];
  double communication_amount[2] = { 0 };
  SD_workstation_t workstation_list[1];
  /* initialization of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment("./faulty_host.xml");
 
  /* creation of a single task that will poorly fail when the workstation
   * will stop */
  XBT_INFO("First test: COMP_SEQ task");
  task = SD_task_create_comp_seq("Poor task", NULL, 2e10);
  SD_task_watch(task, SD_FAILED);
  SD_task_watch(task, SD_DONE);

  XBT_INFO("Schedule task '%s' on workstation 'Faulty Host'",
           SD_task_get_name(task));

  SD_task_schedulel(task, 1, SD_workstation_get_by_name("Faulty Host"));

  SD_simulate(-1.0);

  SD_task_dump(task);

  XBT_INFO("Task '%s' has failed. %.f flops remain to be done",
           SD_task_get_name(task),
           SD_task_get_remaining_amount(task));

  XBT_INFO("let's unschedule task '%s' and reschedule it on the 'Safe Host'",
           SD_task_get_name(task));
  SD_task_unschedule(task);
  SD_task_schedulel(task, 1, SD_workstation_get_by_name("Safe Host"));

  XBT_INFO("Run the simulation again");
  SD_simulate(-1.0);

  SD_task_dump(task);
  XBT_INFO("Task '%s' start time: %f, finish time: %f",
      SD_task_get_name(task),
      SD_task_get_start_time(task),
      SD_task_get_finish_time(task));

  SD_task_destroy(task);
  task=NULL;

  XBT_INFO("Second test: NON TYPED task");

  task = SD_task_create("Poor parallel task", NULL, 2e10);
  SD_task_watch(task, SD_FAILED);
  SD_task_watch(task, SD_DONE);

  computation_amount[0] = 2e10;

  XBT_INFO("Schedule task '%s' on workstation 'Faulty Host'",
             SD_task_get_name(task));

  workstation_list[0] = SD_workstation_get_by_name("Faulty Host");
  SD_task_schedule(task, 1, workstation_list,
          computation_amount, communication_amount,-1);

  SD_simulate(-1.0);

  SD_task_dump(task);

  XBT_INFO("Task '%s' has failed. %.f flops remain to be done",
            SD_task_get_name(task),
            SD_task_get_remaining_amount(task));

  XBT_INFO("let's unschedule task '%s' and reschedule it on the 'Safe Host'",
           SD_task_get_name(task));
  SD_task_unschedule(task);

  workstation_list[0] = SD_workstation_get_by_name("Safe Host");

  SD_task_schedule(task, 1, workstation_list,
                   computation_amount, communication_amount,-1);

  XBT_INFO("Run the simulation again");
  SD_simulate(-1.0);

  SD_task_dump(task);
  XBT_INFO("Task '%s' start time: %f, finish time: %f",
           SD_task_get_name(task),
           SD_task_get_start_time(task),
           SD_task_get_finish_time(task));

  SD_task_destroy(task);
  SD_exit();
  return 0;
}
