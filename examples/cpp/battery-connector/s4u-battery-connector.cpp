/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use the battery as a connector.
   A connector has no capacity and only delivers as much power as it receives
   with a transfer efficiency of 100%.
   A solar panel is connected to the connector and power a host.
*/

#include "simgrid/plugins/battery.hpp"
#include "simgrid/plugins/chiller.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/solar_panel.hpp"
#include "simgrid/s4u.hpp"
#include <math.h>
#include <simgrid/s4u/Actor.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_chiller_solar, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;
namespace sp  = simgrid::plugins;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg_host_energy_plugin_init();

  auto myhost1 = e.host_by_name("MyHost1");

  auto connector     = sp::Battery::init();
  auto solar_panel = sp::SolarPanel::init("Solar Panel", 1, 1, 200, 0, 1e3);

  connector->set_load("Solar Panel", solar_panel->get_power() * -1);
  connector->connect_host(myhost1);

  solar_panel->on_this_power_change_cb([connector](sp::SolarPanel *s) {
    connector->set_load("Solar Panel", s->get_power() * -1);
  });

  sg4::Actor::create("manager", myhost1, [&myhost1, & solar_panel, &connector]{
    XBT_INFO("Solar Panel power = %.2fW, MyHost1 power = %.2fW. The Solar Panel provides more than needed.", solar_panel->get_power(), sg_host_get_current_consumption(myhost1));
    simgrid::s4u::this_actor::sleep_for(100);
    XBT_INFO("Energy consumption MyHost1: %.2fkJ, Energy from the Solar Panel %.2fkJ", sg_host_get_consumed_energy(myhost1) / 1e3, connector->get_energy_provided() / 1e3);

    solar_panel->set_solar_irradiance(100);
    XBT_INFO("Solar Panel power = %.2fW, MyHost1 power = %.2fW. The Solar Panel provides exactly what is needed.", solar_panel->get_power(), sg_host_get_current_consumption(myhost1));
    double last_measure_host_energy = sg_host_get_consumed_energy(myhost1);
    double last_measure_connector_energy = connector->get_energy_provided();

    simgrid::s4u::this_actor::sleep_for(100);
    XBT_INFO("Energy consumption MyHost1: %.2fkJ, Energy from the Solar Panel %.2fkJ", (sg_host_get_consumed_energy(myhost1) - last_measure_host_energy) / 1e3, (connector->get_energy_provided() - last_measure_connector_energy) / 1e3);

    XBT_INFO("MyHost1 executes something for 100s. The Solar Panel does not provide enough energy.");
    last_measure_host_energy = sg_host_get_consumed_energy(myhost1);
    last_measure_connector_energy = connector->get_energy_provided();
    myhost1->execute(100 * myhost1->get_speed());
    XBT_INFO("Energy MyHost1: %.2fkJ, Energy from the Solar Panel %.2fkJ", (sg_host_get_consumed_energy(myhost1) - last_measure_host_energy) / 1e3, (connector->get_energy_provided() - last_measure_connector_energy) / 1e3);
  });

  e.run();
  return 0;
}
