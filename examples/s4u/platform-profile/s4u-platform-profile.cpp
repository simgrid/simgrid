/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

/* This example demonstrates how to attach a profile to an host or a link,
 * to specify external changes to the resource speed. The first way to do
 * so is to use a file in the XML, while the second is to use the programmatic interface.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_platform_profile, "Messages specific for this s4u example");

/* Main function of the Yielder process */
static void watcher()
{
  const simgrid::s4u::Host* jupiter = simgrid::s4u::Host::by_name("Jupiter");
  const simgrid::s4u::Host* fafard  = simgrid::s4u::Host::by_name("Fafard");
  const simgrid::s4u::Link* link1   = simgrid::s4u::Link::by_name("1");
  const simgrid::s4u::Link* link2   = simgrid::s4u::Link::by_name("2");

  for (int i = 0; i < 10; i++) {
    XBT_INFO("Fafard: %.0fGflops, Jupiter: % 3.0fGflops, Link1: (%.2fMB/s %.0fms), Link2: (%.2fMB/s %.0fms)",
             fafard->get_speed() * fafard->get_available_speed() / 1000000,
             jupiter->get_speed() * jupiter->get_available_speed() / 1000000, link1->get_bandwidth() / 1000,
             link1->get_latency() * 1000, link2->get_bandwidth() / 1000, link2->get_latency() * 1000);
    simgrid::s4u::this_actor::sleep_for(1);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml deployment.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("watcher", simgrid::s4u::Host::by_name("Tremblay"), watcher);

  e.run(); /* - Run the simulation */

  return 0;
}
