/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void dvfs()
{
  sg4::Host* host1 = sg4::Host::by_name("MyHost1");
  sg4::Host* host2 = sg4::Host::by_name("MyHost2");

  XBT_INFO("Energetic profile: %s", host1->get_property("wattage_per_state"));
  XBT_INFO("Initial peak speed=%.0E flop/s; Energy dissipated =%.0E J", host1->get_speed(),
           sg_host_get_consumed_energy(host1));

  double start = sg4::Engine::get_clock();
  XBT_INFO("Sleep for 10 seconds");
  sg4::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E; Energy dissipated=%.2f J",
           sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  // Execute something
  start             = sg4::Engine::get_clock();
  double flopAmount = 100E6;
  XBT_INFO("Run a computation of %.0E flops", flopAmount);
  sg4::this_actor::execute(flopAmount);
  XBT_INFO(
      "Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; Current consumption: from %.0fW to %.0fW"
      " depending on load; Energy dissipated=%.0f J",
      sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_wattmin_at(host1, host1->get_pstate()),
      sg_host_get_wattmax_at(host1, host1->get_pstate()), sg_host_get_consumed_energy(host1));

  // ========= Change power peak =========
  int pstate = 2;
  host1->set_pstate(pstate);
  XBT_INFO("========= Requesting pstate %d (speed should be of %.0E flop/s and is of %.0E flop/s)", pstate,
           host1->get_pstate_speed(pstate), host1->get_speed());

  // Run another computation
  start = sg4::Engine::get_clock();
  XBT_INFO("Run a computation of %.0E flops", flopAmount);
  sg4::this_actor::execute(flopAmount);
  XBT_INFO("Computation done (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  start = sg4::Engine::get_clock();
  XBT_INFO("Sleep for 4 seconds");
  sg4::this_actor::sleep_for(4);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));

  // =========== Turn the other host off ==========
  XBT_INFO("Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 dissipated %.0f J so far.",
           sg_host_get_consumed_energy(host2));
  host2->turn_off();
  start = sg4::Engine::get_clock();
  sg4::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping (duration: %.2f s). Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
           sg4::Engine::get_clock() - start, host1->get_speed(), sg_host_get_consumed_energy(host1));
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);
  e.host_by_name("MyHost1")->add_actor("dvfs_test", dvfs);

  e.run();

  XBT_INFO("End of simulation.");

  return 0;
}
