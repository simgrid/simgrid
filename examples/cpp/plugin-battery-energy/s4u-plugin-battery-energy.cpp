/* Copyright (c) 2003-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/battery.hpp>
#include <simgrid/plugins/energy.h>
#include <simgrid/s4u.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_energy, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void manager()
{
  auto battery = simgrid::plugins::Battery::init("Battery", 0.8, -300, 300, 0.9, 0.9, 10, 1000);

  auto e      = sg4::this_actor::get_engine();
  auto* host1 = e->host_by_name("MyHost1");
  auto* host2 = e->host_by_name("MyHost2");
  auto* host3 = e->host_by_name("MyHost3");

  battery->schedule_handler(
      0.2, simgrid::plugins::Battery::DISCHARGE, simgrid::plugins::Battery::Handler::PERSISTANT,
      [&battery, &host1, &host2, &host3]() {
        XBT_INFO("Handler -> Battery low: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
                 battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
                 battery->get_energy_provided(), battery->get_energy_consumed());
        XBT_INFO("Disconnecting hosts %s and %s", host1->get_cname(), host2->get_cname());
        battery->connect_host(host1, false);
        battery->connect_host(host2, false);
        XBT_INFO("Energy consumed this far by: %s: %fJ, %s: %fJ, %s: %fJ", host1->get_cname(),
                 sg_host_get_consumed_energy(host1), host2->get_cname(), sg_host_get_consumed_energy(host2),
                 host3->get_cname(), sg_host_get_consumed_energy(host3));
      });

  XBT_INFO("Battery state: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
           battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
           battery->get_energy_provided(), battery->get_energy_consumed());
  XBT_INFO("Connecting hosts %s and %s to the battery", host1->get_cname(), host2->get_cname());
  battery->connect_host(host1);
  battery->connect_host(host2);

  double flops = 1e9;
  XBT_INFO("Host %s will now execute %f flops", host1->get_cname(), flops);
  host2->execute(flops);

  sg4::this_actor::sleep_until(200);
  XBT_INFO("Battery state: SoC: %f SoH: %f Energy stored: %fJ Energy provided: %fJ Energy consumed %fJ",
           battery->get_state_of_charge(), battery->get_state_of_health(), battery->get_energy_stored(),
           battery->get_energy_provided(), battery->get_energy_consumed());
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  // if you plan to use Battery::connect_host you have to init the energy plugin at start.
  sg_host_energy_plugin_init();
  e.load_platform(argv[1]);

  e.host_by_name("MyHost1")->add_actor("manager", manager);

  e.run();
  return 0;
}
