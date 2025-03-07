/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include "NetZone_test.hpp" // CreateHost callback
#include "simgrid/kernel/routing/FatTreeZone.hpp"
#include "simgrid/s4u/Engine.hpp"

TEST_CASE("kernel::routing::FatTreeZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");
  REQUIRE(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1, 2}, {1, 2}, 1e9, 10,
                                                    simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}));
}

TEST_CASE("kernel::routing::FatTreeZone: Invalid params", "")
{
  simgrid::s4u::Engine e("test");
  
  SECTION("0 levels")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 0, {4, 4}, {1, 2}, {1, 2}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Invalid down links")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4}, {1, 2}, {1, 2}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Invalid up links")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1}, {1, 2}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Invalid link count")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1, 2}, {1}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Down links with zeroes")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 0}, {1, 2}, {1, 2}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Up links with zeroes")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {0, 2}, {1, 2}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Link count with zeroes")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1, 2}, {1, 0}, 1e9, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("0 bandwidth")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1, 2}, {1, 2}, 0, 10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }

  SECTION("Negative latency")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()->add_netzone_fatTree("test", 2, {4, 4}, {1, 2}, {1, 2}, 1e9, -10,
                                                                simgrid::s4u::Link::SharingPolicy::SHARED)->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
}
