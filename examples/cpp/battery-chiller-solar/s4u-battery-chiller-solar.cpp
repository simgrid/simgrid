/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example combine the battery plugin, the chiller plugin and the solar
   panel plugin. It illustrates how to use them together to evaluate the amount
   of brown energy (from the electrical grid) and green energy (from the solar
   panel) consumed by several machines.

   In this scenario we have two host placed in a room.
   The room is maintained at 24°C by a chiller, powered by the electrical grid
   and consumes brown energy.
   The two hosts are powered by a battery when available, and the electrical
   grid otherwise. The battery is charged by a solar panel.

   We simulate two days from 00h00 to 00h00.
   The solar panel generates power from 8h to 20h with a peak at 14h.
   During the simulation, when the charge of the battery goes:
    - below 75% the solar panel is connected to the battery
    - above 80% the solar panel is disconnected from the battery
    - below 20% the hosts are disconnected from the battery
    - above 25% the hosts are connected to the battery

   The two hosts are always idle, except from 12h to 16h on the first day.
*/

#include "simgrid/plugins/battery.hpp"
#include "simgrid/plugins/chiller.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/solar_panel.hpp"
#include "simgrid/s4u.hpp"
#include <math.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_chiller_solar, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;
namespace sp  = simgrid::plugins;

static void irradiance_manager(sp::SolarPanelPtr solar_panel)
{
  int time         = 0;
  int time_step    = 10;
  double amplitude = 1000 / 2.0;
  double period    = 24 * 60 * 60;
  double shift     = 16 * 60 * 60;
  double irradiance;
  while (true) {
    irradiance = amplitude * sin(2 * M_PI * (time + shift) / period);
    irradiance = irradiance < 0 ? 0 : irradiance;
    solar_panel->set_solar_irradiance(irradiance);
    sg4::this_actor::sleep_for(time_step);
    time += time_step;
  }
}

static void host_job_manager(double start, double duration)
{
  sg4::this_actor::sleep_until(start);
  sg4::this_actor::get_host()->execute(duration * sg4::this_actor::get_host()->get_speed());
}

static void end_manager(sp::BatteryPtr b)
{
  sg4::this_actor::sleep_until(86400 * 2);
  for (auto& handler : b->get_handlers())
    b->delete_handler(handler);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg_host_energy_plugin_init();

  auto myhost1 = e.host_by_name("MyHost1");
  auto myhost2 = e.host_by_name("MyHost2");

  auto battery     = sp::Battery::init("Battery", 0.2, -1e3, 1e3, 0.9, 0.9, 2000, 1000);
  auto chiller     = sp::Chiller::init("Chiller", 50, 1006, 0.2, 0.9, 24, 24, 1e3);
  auto solar_panel = sp::SolarPanel::init("Solar Panel", 1.1, 0.9, 0, 0, 1e3);
  chiller->add_host(myhost1);
  chiller->add_host(myhost2);
  solar_panel->on_this_power_change_cb(
      [battery](sp::SolarPanel* s) { battery->set_load("Solar Panel", s->get_power() * -1); });

  // These handlers connect/disconnect the solar panel and the hosts depending on the state of charge of the battery
  battery->schedule_handler(0.8, sp::Battery::CHARGE, sp::Battery::Handler::PERSISTANT,
                            [battery]() { battery->set_load("Solar Panel", false); });
  battery->schedule_handler(0.75, sp::Battery::DISCHARGE, sp::Battery::Handler::PERSISTANT,
                            [battery]() { battery->set_load("Solar Panel", true); });
  battery->schedule_handler(0.2, sp::Battery::DISCHARGE, sp::Battery::Handler::PERSISTANT,
                            [battery, &myhost1, &myhost2]() {
                              battery->connect_host(myhost1, false);
                              battery->connect_host(myhost2, false);
                            });
  battery->schedule_handler(0.25, sp::Battery::CHARGE, sp::Battery::Handler::PERSISTANT,
                            [battery, &myhost1, &myhost2]() {
                              battery->connect_host(myhost1);
                              battery->connect_host(myhost2);
                            });

  /* Handlers create simulation events preventing the simulation from finishing
     To avoid this behaviour this actor sleeps for 2 days and then delete all handlers
  */
  sg4::Actor::create("end_manager", myhost1, end_manager, battery);

  // This actor updates the solar irradiance of the solar panel
  sg4::Actor::create("irradiance_manager", myhost1, irradiance_manager, solar_panel)->daemonize();

  // These actors start a job on a host for a specific duration
  sg4::Actor::create("host_job_manager", myhost1, host_job_manager, 12 * 60 * 60, 4 * 60 * 60);
  sg4::Actor::create("host_job_manager", myhost2, host_job_manager, 12 * 60 * 60, 4 * 60 * 60);

  e.run();
  XBT_INFO("State of charge of the battery: %0.1f%%", battery->get_state_of_charge() * 100);
  XBT_INFO(
      "Energy consumed by the hosts (green / brown): %.2fMJ "
      "/ %.2fMJ",
      battery->get_energy_provided() / 1e6,
      (sg_host_get_consumed_energy(myhost1) + sg_host_get_consumed_energy(myhost2) - battery->get_energy_provided()) /
          1e6);
  XBT_INFO("Energy consumed by the chiller (brown): %.2fMJ", chiller->get_energy_consumed() / 1e6);
  return 0;
}
