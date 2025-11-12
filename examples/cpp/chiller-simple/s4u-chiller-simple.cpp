/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/chiller.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(chiller_simple, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void display_chiller(simgrid::plugins::ChillerPtr c)
{
  XBT_INFO("%s: Power: %.2fW T_in: %.2f°C Energy consumed: %.2fJ", c->get_cname(), c->get_power(), c->get_temp_in(),
           c->get_energy_consumed());
}

static void manager(simgrid::plugins::ChillerPtr c)
{
  display_chiller(c);

  sg4::this_actor::sleep_for(c->get_time_to_goal_temp());
  XBT_INFO("The input temperature is now equal to the goal temperature. After this point the Chiller will compensate "
           "heat with electrical power.");
  display_chiller(c);

  sg4::this_actor::sleep_for(1);
  display_chiller(c);

  XBT_INFO("Let's compute something.");
  sg4::this_actor::execute(1e10);
  XBT_INFO("Computation done.");
  display_chiller(c);
  XBT_INFO("Now let's stress the chiller by decreasing the goal temperature to 23°C.");
  c->set_goal_temp(23);

  sg4::this_actor::sleep_for(1);
  display_chiller(c);

  sg4::this_actor::sleep_for(c->get_time_to_goal_temp());
  XBT_INFO("The input temperature is back to the goal temperature.");
  display_chiller(c);

  sg4::this_actor::sleep_for(1);
  display_chiller(c);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg_host_energy_plugin_init();

  auto chiller = simgrid::plugins::Chiller::init("Chiller", 294, 1006, 0.2, 0.9, 23, 24, 1000);
  chiller->add_host(e.host_by_name("MyHost1"));
  chiller->add_host(e.host_by_name("MyHost2"));
  chiller->add_host(e.host_by_name("MyHost3"));
  e.host_by_name("MyHost1")->add_actor("manager", manager, chiller);

  e.run();
  return 0;
}
