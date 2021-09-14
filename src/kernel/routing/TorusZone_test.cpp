/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "NetZone_test.hpp" // CreateHost callback
#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/s4u/Engine.hpp"

TEST_CASE("kernel::routing::TorusZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");
  simgrid::s4u::ClusterCallbacks callbacks(CreateHost{});
  REQUIRE(create_torus_zone("test", e.get_netzone_root(), {3, 3, 3}, callbacks, 1e9, 10,
                            simgrid::s4u::Link::SharingPolicy::SHARED));
}

TEST_CASE("kernel::routing::TorusZone: Invalid params", "")
{
  simgrid::s4u::Engine e("test");
  simgrid::s4u::ClusterCallbacks callbacks(CreateHost{});

  SECTION("Empty dimensions")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.get_netzone_root(), {}, callbacks, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("One 0 dimension")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.get_netzone_root(), {3, 0, 2}, callbacks, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("Invalid bandwidth")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.get_netzone_root(), {3, 2, 2}, callbacks, 0, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("Invalid latency")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.get_netzone_root(), {3, 2, 2}, callbacks, 1e9, -10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
}
