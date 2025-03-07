/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include "NetZone_test.hpp" // CreateHost callback
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/s4u/Engine.hpp"

TEST_CASE("kernel::routing::DragonflyZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");
  REQUIRE(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {5, 1}, 2, 1e9, 10,
                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}));
}

TEST_CASE("kernel::routing::DragonflyZone: Invalid params", "")
{
  simgrid::s4u::Engine e("test");
  
  SECTION("0 nodes")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {5, 1}, 0, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 groups")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {0, 4}, {4, 3}, {5, 1}, 2,  1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
  SECTION("0 groups links")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 0}, {4, 3}, {5, 1}, 2, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 chassis")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {0, 3}, {5, 1}, 2, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 chassis links")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 0}, {5, 1}, 2, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 routers")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {0, 1}, 2, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 routers links")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {5, 0}, 2, 1e9,
                                            10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 bandwidth")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {5, 1}, 2,  0, 10,
                                            simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Negative latency")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_dragonfly("test", {3, 4}, {4, 3}, {5, 1}, 2, 1e9,
                                            -10, simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
}
