/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_degradation, "Messages specific for this s4u example");

static void manager()
{
  const auto* battery = simgrid::s4u::this_actor::get_host();
  double power = 100;
  while (sg_battery_get_state_of_health(battery) > 0) {
    XBT_INFO("%f,%f,SoC", simgrid::s4u::Engine::get_clock(), sg_battery_get_state_of_charge(battery));
    XBT_INFO("%f,%f,SoH", simgrid::s4u::Engine::get_clock(), sg_battery_get_state_of_health(battery));
    sg_battery_set_power(battery, power);
    simgrid::s4u::this_actor::sleep_until(sg_battery_get_next_event_date(battery));
    power *= -1;
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg_battery_plugin_init();

  simgrid::s4u::Actor::create("manager", e.get_all_hosts()[0], manager);
  e.run();
  return 0;
}
