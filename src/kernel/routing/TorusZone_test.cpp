/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include "NetZone_test.hpp" // CreateHost callback
#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/s4u/Engine.hpp"

TEST_CASE("kernel::routing::TorusZone: Creating Zone", "")
{
  simgrid::s4u::Engine e("test");
  REQUIRE(e.get_netzone_root()
              ->add_netzone_torus("test", {3, 3, 3}, 1e9, 10, simgrid::s4u::Link::SharingPolicy::SHARED)
              ->set_host_cb(CreateHost{}));
}

TEST_CASE("kernel::routing::TorusZone: Invalid params", "")
{
  simgrid::s4u::Engine e("test");
  
  SECTION("Empty dimensions")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()
                          ->add_netzone_torus("test", {}, 1e9, 10, simgrid::s4u::Link::SharingPolicy::SHARED)
                          ->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
  SECTION("One 0 dimension")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()
                          ->add_netzone_torus("test", {3, 0, 2}, 1e9, 10, simgrid::s4u::Link::SharingPolicy::SHARED)
                          ->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
  SECTION("Invalid bandwidth")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()
                          ->add_netzone_torus("test", {3, 2, 2}, 0, 10, simgrid::s4u::Link::SharingPolicy::SHARED)
                          ->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
  SECTION("Invalid latency")
  {
    REQUIRE_THROWS_AS(e.get_netzone_root()
                          ->add_netzone_torus("test", {3, 2, 2}, 1e9, -10, simgrid::s4u::Link::SharingPolicy::SHARED)
                          ->set_host_cb(CreateHost{}),
                      std::invalid_argument);
  }
}
