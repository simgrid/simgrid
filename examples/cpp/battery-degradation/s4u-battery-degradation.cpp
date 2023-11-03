/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/battery.hpp"
#include "simgrid/s4u.hpp"
#include <memory>
#include <simgrid/s4u/Actor.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(battery_degradation, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  auto battery = simgrid::plugins::Battery::init("Battery", 0.8, -200, 200, 0.9, 0.9, 10, 100);

  battery->set_load("load", 100.0);

  auto handler1 = battery->schedule_handler(
      0.2, simgrid::plugins::Battery::DISCHARGE, simgrid::plugins::Battery::Handler::PERSISTANT, [&battery]() {
        XBT_INFO("%f,%f,SoC", simgrid::s4u::Engine::get_clock(), battery->get_state_of_charge());
        XBT_INFO("%f,%f,SoH", simgrid::s4u::Engine::get_clock(), battery->get_state_of_health());
        battery->set_load("load", -100.0);
      });

  std::shared_ptr<simgrid::plugins::Battery::Handler> handler2;
  handler2 = battery->schedule_handler(
      0.8, simgrid::plugins::Battery::CHARGE, simgrid::plugins::Battery::Handler::PERSISTANT,
      [&battery, &handler1, &handler2]() {
        XBT_INFO("%f,%f,SoC", simgrid::s4u::Engine::get_clock(), battery->get_state_of_charge());
        XBT_INFO("%f,%f,SoH", simgrid::s4u::Engine::get_clock(), battery->get_state_of_health());
        if (battery->get_state_of_health() < 0.1) {
          battery->delete_handler(handler1);
          battery->delete_handler(handler2);
        }
        battery->set_load("load", 100.0);
      });

  e.run();
  return 0;
}
