/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_simple, "Messages specific for this s4u example");

static void manager()
{
  const auto* battery  = simgrid::s4u::Engine::get_instance()->host_by_name("battery");
  double consumption_w = 200;

  XBT_INFO("Initial Battery: SoC: %f SoH: %f Capacity (Total): %fWh Capacity (Usable): %fWh P: %f",
           sg_battery_get_state_of_charge(battery), sg_battery_get_state_of_health(battery),
           sg_battery_get_capacity(battery),
           sg_battery_get_capacity(battery) *
               (sg_battery_get_state_of_charge_max(battery) - sg_battery_get_state_of_charge_min(battery)),
           sg_battery_get_power(battery));
  double start = simgrid::s4u::Engine::get_clock();
  sg_battery_set_power(battery, consumption_w);
  XBT_INFO("Battery power set to: %fW", consumption_w);
  double end = sg_battery_get_next_event_date(battery);
  XBT_INFO("The battery will be depleted at: %f", end);
  simgrid::s4u::this_actor::sleep_until(end);
  XBT_INFO("Energy consumed : %fJ (%fWh)", (end - start) * consumption_w, (end - start) * consumption_w / 3600);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg_battery_plugin_init();

  simgrid::s4u::Actor::create("manager", e.host_by_name("host1"), manager);
  e.run();
  return 0;
}
