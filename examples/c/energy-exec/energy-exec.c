/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/plugins/energy.h"
#include "xbt/asserts.h"
#include "xbt/config.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_exec, "Messages specific for this example");

static void dvfs(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  sg_host_t host = sg_host_by_name("MyHost1");

  XBT_INFO("Energetic profile: %s", sg_host_get_property_value(host, "wattage_per_state"));
  XBT_INFO("Initial peak speed=%.0E flop/s; Energy dissipated =%.0E J", sg_host_speed(host),
           sg_host_get_consumed_energy(host));

  double start = simgrid_get_clock();
  XBT_INFO("Sleep for 10 seconds");
  sg_actor_sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E; Energy dissipated=%.2f J",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_consumed_energy(host));

  // Run a task
  start = simgrid_get_clock();
  XBT_INFO("Run a task of %.0E flops", 100E6);
  sg_actor_execute(100E6);
  XBT_INFO("Task done (duration: %.2f s). Current peak speed=%.0E flop/s; Current consumption: from %.0fW to %.0fW"
           " depending on load; Energy dissipated=%.0f J",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_wattmin_at(host, sg_host_get_pstate(host)),
           sg_host_get_wattmax_at(host, sg_host_get_pstate(host)), sg_host_get_consumed_energy(host));

  // ========= Change power peak =========
  int pstate = 2;
  sg_host_set_pstate(host, pstate);
  XBT_INFO("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           sg_host_get_pstate_speed(host, pstate), sg_host_speed(host));

  // Run a second task
  start = simgrid_get_clock();
  XBT_INFO("Run a task of %.0E flops", 100E6);
  sg_actor_execute(100E6);
  XBT_INFO("Task done (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_consumed_energy(host));

  start = simgrid_get_clock();
  XBT_INFO("Sleep for 4 seconds");
  sg_actor_sleep_for(4);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_consumed_energy(host));

  // =========== Turn the other host off ==========
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 dissipated %.0f J so far.",
           sg_host_get_consumed_energy(sg_host_by_name("MyHost2")));
  sg_host_turn_off(sg_host_by_name("MyHost2"));
  start = simgrid_get_clock();
  sg_actor_sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid_get_clock() - start, sg_host_speed(host), sg_host_get_consumed_energy(host));
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  sg_cfg_set_string("plugin", "host_energy");

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  simgrid_load_platform(argv[1]);
  sg_actor_create("dvfs_test", sg_host_by_name("MyHost1"), dvfs, 0, NULL);

  simgrid_run();

  XBT_INFO("End of simulation");

  return 0;
}
