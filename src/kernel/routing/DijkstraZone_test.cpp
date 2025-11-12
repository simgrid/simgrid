/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include "simgrid/kernel/routing/DijkstraZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"

TEST_CASE("kernel::routing::DijkstraZone (cached): mix new routes and hosts", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = e.get_netzone_root()->add_netzone_dijkstra("test", true);

  const simgrid::s4u::Host* nic  = zone->add_host("nic", 1e9)->seal();
  const simgrid::s4u::Link* link = zone->add_link("my_link", 1e6)->seal();
  for (int i = 0; i < 10; i++) {
    std::string cpu_name          = "CPU" + std::to_string(i);
    const simgrid::s4u::Host* cpu = zone->add_host(cpu_name, 1e9)->seal();
    REQUIRE_NOTHROW(zone->add_route(cpu, nic, {link}));
  }
}

TEST_CASE("kernel::routing::DijkstraZone (not cached): mix new routes and hosts", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = e.get_netzone_root()->add_netzone_dijkstra("test", false);

  const simgrid::s4u::Host* nic  = zone->add_host("nic", 1e9)->seal();
  const simgrid::s4u::Link* link = zone->add_link("my_link", 1e6)->seal();
  for (int i = 0; i < 10; i++) {
    std::string cpu_name          = "CPU" + std::to_string(i);
    const simgrid::s4u::Host* cpu = zone->add_host(cpu_name, 1e9)->seal();
    REQUIRE_NOTHROW(zone->add_route(cpu, nic,{link}));
  }
}
