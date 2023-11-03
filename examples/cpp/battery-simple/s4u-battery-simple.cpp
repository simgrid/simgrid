/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/s4u.hpp"
#include <simgrid/s4u/Actor.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_simple, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  auto battery = simgrid::plugins::Battery::init("Battery", 0.8, -100, 100, 0.9, 0.9, 10, 1000);

  XBT_INFO("Initial state: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
           battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
           battery->get_energy_provided(), battery->get_energy_consumed());

  /* This power is beyond the nominal values of the battery
   * see documentation for more info
   */
  double load_w = 150;
  battery->set_load("load", load_w);
  XBT_INFO("Set load to %fW", load_w);

  battery->schedule_handler(
      0.2, simgrid::plugins::Battery::DISCHARGE, simgrid::plugins::Battery::Handler::PERSISTANT, [&battery, &load_w]() {
        XBT_INFO("Discharged state: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
                 battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
                 battery->get_energy_provided(), battery->get_energy_consumed());
        battery->set_load("load", -load_w);
        XBT_INFO("Set load to %fW", -load_w);
      });

  battery->schedule_handler(
      0.8, simgrid::plugins::Battery::CHARGE, simgrid::plugins::Battery::Handler::PERSISTANT, [&battery]() {
        XBT_INFO("Charged state: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
                 battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
                 battery->get_energy_provided(), battery->get_energy_consumed());
        XBT_INFO("Set load to %fW", 0.0);
      });

  e.run();

  return 0;
}