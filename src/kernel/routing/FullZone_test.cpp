/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/kernel/resource/LinkImpl.hpp"

TEST_CASE("kernel::routing::FullZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");

  REQUIRE(simgrid::s4u::create_full_zone("test"));
}

TEST_CASE("kernel::routing::FullZone: mix new routes and hosts", "[bug]")
{
  simgrid::s4u::Engine e("test");
  auto* zone = simgrid::s4u::create_full_zone("test");

  const simgrid::s4u::Host* nic  = zone->create_host("nic", 1e9)->seal();
  const simgrid::s4u::Link* link = zone->create_link("my_link", 1e6)->seal();
  for (int i = 0; i < 10; i++) {
    std::string cpu_name          = "CPU" + std::to_string(i);
    const simgrid::s4u::Host* cpu = zone->create_host(cpu_name, 1e9)->seal();
    REQUIRE_NOTHROW(zone->add_route(cpu->get_netpoint(), nic->get_netpoint(), nullptr, nullptr,
                                    {simgrid::s4u::LinkInRoute(link)}, true));
  }
}
