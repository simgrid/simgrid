/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "NetZone_test.hpp" // CreateHost callback
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/s4u/Engine.hpp"

TEST_CASE("kernel::routing::DragonflyZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");
  simgrid::s4u::ClusterCallbacks callbacks(CreateHost{});
  REQUIRE(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {5, 1}, 2}, callbacks, 1e9, 10,
                                simgrid::s4u::Link::SharingPolicy::SHARED));
}

TEST_CASE("kernel::routing::DragonflyZone: Invalid params", "")
{
  simgrid::s4u::Engine e("test");
  simgrid::s4u::ClusterCallbacks callbacks(CreateHost{});

  SECTION("0 nodes")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {5, 1}, 0}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 groups")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{0, 4}, {4, 3}, {5, 1}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("0 groups links")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 0}, {4, 3}, {5, 1}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 chassis")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {0, 3}, {5, 1}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 chassis links")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 0}, {5, 1}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 routers")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {0, 1}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 routers links")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {5, 0}, 2}, callbacks, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 bandwidth")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {5, 1}, 2}, callbacks, 0, 10,
                                            simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Negative latency")
  {
    REQUIRE_THROWS_AS(create_dragonfly_zone("test", e.get_netzone_root(), {{3, 4}, {4, 3}, {5, 1}, 2}, callbacks, 1e9,
                                            -10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
}
