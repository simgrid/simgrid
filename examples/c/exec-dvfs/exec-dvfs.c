/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "xbt/asserts.h"
#include "xbt/config.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Pstate properties test");

static void dvfs(int argc, char* argv[])
{
  sg_host_t host = sg_host_self();

  unsigned long nb = sg_host_get_nb_pstates(host);
  XBT_INFO("Count of Processor states=%lu", nb);

  double current_peak = sg_host_get_speed(host);
  XBT_INFO("Current power peak=%f", current_peak);

  sg_actor_execute(100E6);

  double task_time = simgrid_get_clock();
  XBT_INFO("Task1 simulation time: %e", task_time);

  // Change power peak
  unsigned long new_pstate = 2;
  xbt_assert(new_pstate < nb, "Cannot set the host %s at pstate %lu because it only provides %lu pstates.",
             sg_host_get_name(host), new_pstate, nb);

  double peak_at = sg_host_get_pstate_speed(host, new_pstate);
  XBT_INFO("Changing power peak value to %f (at index %lu)", peak_at, new_pstate);

  sg_host_set_pstate(host, new_pstate);

  current_peak = sg_host_get_speed(host);
  XBT_INFO("Current power peak=%f", current_peak);

  sg_actor_execute(100E6);

  task_time = simgrid_get_clock() - task_time;
  XBT_INFO("Task2 simulation time: %e", task_time);

  // Verify the default pstate is set to 0
  host    = sg_host_by_name("MyHost2");
  unsigned long nb2 = sg_host_get_nb_pstates(host);
  XBT_INFO("Count of Processor states=%lu", nb2);

  double current_peak2 = sg_host_get_speed(host);
  XBT_INFO("Current power peak=%f", current_peak2);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("dvfs_test", sg_host_by_name("MyHost1"), dvfs, 0, NULL);
  sg_actor_create("dvfs_test", sg_host_by_name("MyHost2"), dvfs, 0, NULL);

  simgrid_run();

  XBT_INFO("Total simulation time: %e", simgrid_get_clock());

  return 0;
}
