/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/s4u.hpp"

/* This example demonstrates how to attach a profile to a host or a link, to specify external changes to the resource
 * speed. The first way to do so is to use a file in the XML, while the second is to use the programmatic interface. */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_platform_profile, "Messages specific for this s4u example");

static void watcher()
{
  const auto* jupiter  = simgrid::s4u::Host::by_name("Jupiter");
  const auto* fafard   = simgrid::s4u::Host::by_name("Fafard");
  const auto* lilibeth = simgrid::s4u::Host::by_name("Lilibeth");
  const auto* link1    = simgrid::s4u::Link::by_name("1");
  const auto* link2    = simgrid::s4u::Link::by_name("2");

  std::vector<simgrid::s4u::Link*> links;
  double lat = 0;
  jupiter->route_to(fafard, links, &lat);

  std::string path;
  for (const auto* l : links)
    path += (path.empty() ? "" : ", ") + std::string("link '") + l->get_name() + std::string("'");
  XBT_INFO("Path from Jupiter to Fafard: %s (latency: %fs).", path.c_str(), lat);

  for (int i = 0; i < 10; i++) {
    XBT_INFO("Fafard: %.0fMflops, Jupiter: %4.0fMflops, Lilibeth: %3.1fMflops, Link1: (%.2fMB/s %.0fms), Link2: "
             "(%.2fMB/s %.0fms)",
             fafard->get_speed() * fafard->get_available_speed() / 1000000,
             jupiter->get_speed() * jupiter->get_available_speed() / 1000000,
             lilibeth->get_speed() * lilibeth->get_available_speed() / 1000000, link1->get_bandwidth() / 1000,
             link1->get_latency() * 1000, link2->get_bandwidth() / 1000, link2->get_latency() * 1000);
    simgrid::s4u::this_actor::sleep_for(1);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // Add a new host programmatically, and attach a simple speed profile to it (alternate between full and half speed
  // every two seconds
  e.get_netzone_root()
      ->create_host("Lilibeth", 25e6)
      ->set_speed_profile(simgrid::kernel::profile::ProfileBuilder::from_string("lilibeth_profile", R"(
0 1.0
2 0.5
)",
                                                                                2))
      ->seal();

  // Add a watcher of the changes
  simgrid::s4u::Actor::create("watcher", e.host_by_name("Fafard"), watcher);

  e.run();

  return 0;
}
