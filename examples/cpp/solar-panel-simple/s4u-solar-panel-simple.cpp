/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/solar_panel.hpp"
#include "simgrid/s4u.hpp"
#include <simgrid/s4u/Actor.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(solar_panel_simple, "Messages specific for this s4u example");

static void manager()
{
  auto solar_panel = simgrid::plugins::SolarPanel::init("Solar Panel", 10, 0.9, 10, 0, 1e3);
  simgrid::s4u::this_actor::sleep_for(1);
  XBT_INFO("%s: area: %fm² efficiency: %f irradiance: %fW/m² power: %fW", solar_panel->get_cname(),
           solar_panel->get_area(), solar_panel->get_conversion_efficiency(), solar_panel->get_solar_irradiance(),
           solar_panel->get_power());

  solar_panel->set_area(20);
  simgrid::s4u::this_actor::sleep_for(1);
  XBT_INFO("%s: area: %fm² efficiency: %f irradiance: %fW/m² power: %fW", solar_panel->get_cname(),
           solar_panel->get_area(), solar_panel->get_conversion_efficiency(), solar_panel->get_solar_irradiance(),
           solar_panel->get_power());

  solar_panel->set_conversion_efficiency(0.8);
  simgrid::s4u::this_actor::sleep_for(1);
  XBT_INFO("%s: area: %fm² efficiency: %f irradiance: %fW/m² power: %fW", solar_panel->get_cname(),
           solar_panel->get_area(), solar_panel->get_conversion_efficiency(), solar_panel->get_solar_irradiance(),
           solar_panel->get_power());

  solar_panel->set_solar_irradiance(20);
  simgrid::s4u::this_actor::sleep_for(1);
  XBT_INFO("%s: area: %fm² efficiency: %f irradiance: %fW/m² power: %fW", solar_panel->get_cname(),
           solar_panel->get_area(), solar_panel->get_conversion_efficiency(), solar_panel->get_solar_irradiance(),
           solar_panel->get_power());
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("manager", e.host_by_name("Tremblay"), manager);
  e.run();
  return 0;
}
