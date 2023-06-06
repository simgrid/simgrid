/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"

#include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_energy, "Messages specific for this s4u example");

static void manager()
{
  const auto* battery = simgrid::s4u::Engine::get_instance()->host_by_name("battery");
  auto* host1         = simgrid::s4u::Engine::get_instance()->host_by_name("host1");
  auto* host2         = simgrid::s4u::Engine::get_instance()->host_by_name("host2");

  XBT_INFO("Initial Battery: SoC: %f SoH: %f Capacity (Total): %fWh Capacity (Usable): %fWh P: %fW",
           sg_battery_get_state_of_charge(battery), sg_battery_get_state_of_health(battery),
           sg_battery_get_capacity(battery),
           sg_battery_get_capacity(battery) *
               (sg_battery_get_state_of_charge_max(battery) - sg_battery_get_state_of_charge_min(battery)),
           sg_battery_get_power(battery));

  // Start execs on each host
  simgrid::s4u::ExecPtr exec1 = simgrid::s4u::Exec::init();
  exec1->set_flops_amount(1e10);
  exec1->set_host(host1);
  exec1->start();
  simgrid::s4u::ExecPtr exec2 = simgrid::s4u::Exec::init();
  exec2->set_flops_amount(1e10);
  exec2->set_host(host2);
  exec2->start();
  // Set power generation from the battery
  double total_power_w =
      sg_host_get_wattmax_at(host1, host1->get_pstate()) + sg_host_get_wattmax_at(host2, host2->get_pstate());
  sg_battery_set_power(battery, total_power_w);
  XBT_INFO("Battery power set to: %fW (host1) + %fW (host2)", sg_host_get_wattmax_at(host1, host1->get_pstate()),
           sg_host_get_wattmax_at(host2, host2->get_pstate()));
  double end = sg_battery_get_next_event_date(battery);
  XBT_INFO("The battery will be depleted at: %f", end);
  XBT_INFO("Exec1 will be finished in: %f", exec1->get_remaining() / host1->get_speed());
  XBT_INFO("Exec2 will be finished in: %f", exec2->get_remaining() / host2->get_speed());
  simgrid::s4u::this_actor::sleep_until(end);

  // Battery depleted
  XBT_INFO("Battery depleted: SoC: %f SoH: %f P: %fW", sg_battery_get_state_of_charge(battery),
           sg_battery_get_state_of_health(battery), sg_battery_get_power(battery));
  double energy_battery = sg_host_get_consumed_energy(host1) + sg_host_get_consumed_energy(host2);
  XBT_INFO("Pursuing with power from the grid until both execs are finished");
  simgrid::s4u::this_actor::sleep_for(
      std::max(exec1->get_remaining() / host1->get_speed(), exec2->get_remaining() / host2->get_speed()));

  // Execs finished
  double energy_grid = sg_host_get_consumed_energy(host1) + sg_host_get_consumed_energy(host2) - energy_battery;
  XBT_INFO("Energy consumed: Battery: %fJ (%fWh) Grid: %fJ (%fWh)", energy_battery, energy_battery / 3600, energy_grid,
           energy_grid / 3600);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg_host_energy_plugin_init();
  sg_battery_plugin_init();

  simgrid::s4u::Actor::create("manager", e.host_by_name("host1"), manager);
  e.run();
  return 0;
}
