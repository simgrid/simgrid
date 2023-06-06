/* Copyright (c) 2003-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/photovoltaic.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(photovoltaic_simple, "Messages specific for this s4u example");

static void manager()
{
  const auto* pv_panel = simgrid::s4u::Engine::get_instance()->host_by_name("pv_panel");
  std::vector<std::pair<double, double>> solar_irradiance = {{1, 10}, {100, 5}, {200, 20}};
  for (auto [t, s] : solar_irradiance) {
    simgrid::s4u::this_actor::sleep_until(t);
    sg_photovoltaic_set_solar_irradiance(pv_panel, s);
    XBT_INFO("%s power: %f", pv_panel->get_cname(), sg_photovoltaic_get_power(pv_panel));
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg_photovoltaic_plugin_init();
  simgrid::s4u::Actor::create("manager", e.host_by_name("pv_panel"), manager);
  e.run();
  return 0;
}
