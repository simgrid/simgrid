/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/s4u.hpp"
#include <memory>
#include <simgrid/s4u/Actor.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_degradation, "Messages specific for this s4u example");

static void manager()
{
  auto battery = simgrid::plugins::Battery::init("Battery", 0.8, 0.9, 0.9, 10, 100);

  battery->set_load("load", 100);

  auto event1 = battery->create_event(
      0.2, simgrid::plugins::Battery::DISCHARGE,
      [battery]() {
        XBT_INFO("%f,%f,SoC", simgrid::s4u::Engine::get_clock(), battery->get_state_of_charge());
        XBT_INFO("%f,%f,SoH", simgrid::s4u::Engine::get_clock(), battery->get_state_of_health());
        battery->set_load("load", -100);
      },
      true);

  std::shared_ptr<simgrid::plugins::Battery::Event> event2;
  event2 = battery->create_event(
      0.8, simgrid::plugins::Battery::CHARGE,
      [battery, event1, event2]() {
        XBT_INFO("%f,%f,SoC", simgrid::s4u::Engine::get_clock(), battery->get_state_of_charge());
        XBT_INFO("%f,%f,SoH", simgrid::s4u::Engine::get_clock(), battery->get_state_of_health());
        if (battery->get_state_of_health() < 0.1) {
          battery->delete_event(event1);
          battery->delete_event(event2);
        }
        battery->set_load("load", 100);
      },
      true);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("manager", e.host_by_name("MyHost1"), manager);

  e.run();
  return 0;
}
