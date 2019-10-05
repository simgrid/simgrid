/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void dvfs()
{
  simgrid::s4u::Host* host1 = simgrid::s4u::Host::by_name("MyHost1");
  simgrid::s4u::Host* host2 = simgrid::s4u::Host::by_name("MyHost2");

  XBT_INFO("Energetic profile: %s", host1->get_property("wattage_per_state"));
  XBT_INFO("Initial peak speed=%.0E flop/s; Energy dissipated =%.0E J", host1->get_speed(),
           sg_host_get_consumed_energy(host1));

  double start = simgrid::s4u::Engine::get_clock();
  XBT_INFO("Sleep for 10 seconds");
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E; Energy dissipated=%.2f J",
           simgrid::s4u::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  // Execute something
  start             = simgrid::s4u::Engine::get_clock();
  double flopAmount = 100E6;
  XBT_INFO("Run a task of %.0E flops", flopAmount);
  simgrid::s4u::this_actor::execute(flopAmount);
  XBT_INFO("Task done (duration: %.2f s). Current peak speed=%.0E flop/s; Current consumption: from %.0fW to %.0fW"
           " depending on load; Energy dissipated=%.0f J",
           simgrid::s4u::Engine::get_clock() - start, host1->get_speed(),
           sg_host_get_wattmin_at(host1, host1->get_pstate()), sg_host_get_wattmax_at(host1, host1->get_pstate()),
           sg_host_get_consumed_energy(host1));

  // ========= Change power peak =========
  int pstate = 2;
  host1->set_pstate(pstate);
  XBT_INFO("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           host1->get_pstate_speed(pstate), host1->get_speed());

  // Run another task
  start = simgrid::s4u::Engine::get_clock();
  XBT_INFO("Run a task of %.0E flops", flopAmount);
  simgrid::s4u::this_actor::execute(flopAmount);
  XBT_INFO("Task done (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid::s4u::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  start = simgrid::s4u::Engine::get_clock();
  XBT_INFO("Sleep for 4 seconds");
  simgrid::s4u::this_actor::sleep_for(4);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid::s4u::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  // =========== Turn the other host off ==========
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 dissipated %.0f J so far.",
           sg_host_get_consumed_energy(host2));
  host2->turn_off();
  start = simgrid::s4u::Engine::get_clock();
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           simgrid::s4u::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dvfs_test", simgrid::s4u::Host::by_name("MyHost1"), dvfs);

  e.run();

  XBT_INFO("End of simulation.");

  return 0;
}
