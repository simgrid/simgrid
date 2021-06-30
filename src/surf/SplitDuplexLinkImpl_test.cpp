/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/surf/SplitDuplexLinkImpl.hpp"

TEST_CASE("SplitDuplexLink: create", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = simgrid::s4u::create_star_zone("test");

  SECTION("create string")
  {
    simgrid::s4u::Link* link_up;
    simgrid::s4u::Link* link_down;
    simgrid::s4u::SplitDuplexLink* link;
    REQUIRE_NOTHROW(link = zone->create_split_duplex_link("link", "100GBps"));
    REQUIRE(simgrid::s4u::SplitDuplexLink::by_name("link") == link);
    REQUIRE_NOTHROW(link_up = simgrid::s4u::Link::by_name("link_UP"));
    REQUIRE_NOTHROW(link_down = simgrid::s4u::Link::by_name("link_DOWN"));
    REQUIRE(link_up->get_bandwidth() == 100e9);
    REQUIRE(link_down->get_bandwidth() == 100e9);
    REQUIRE(link_up == link->get_link_up());
    REQUIRE(link_down == link->get_link_down());
  }

  SECTION("create double") { REQUIRE_NOTHROW(zone->create_split_duplex_link("link", 10e6)); }
}

TEST_CASE("SplitDuplexLink: sets", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone      = simgrid::s4u::create_star_zone("test");
  auto* link      = zone->create_split_duplex_link("link", 100e6);
  auto* link_up   = link->get_link_up();
  auto* link_down = link->get_link_down();

  SECTION("bandwidth")
  {
    double bw = 1e3;
    link->set_bandwidth(bw);
    REQUIRE(link_up->get_bandwidth() == bw);
    REQUIRE(link_down->get_bandwidth() == bw);
  }

  SECTION("latency")
  {
    double lat = 1e-9;
    link->set_latency(lat);
    REQUIRE(link_up->get_latency() == lat);
    REQUIRE(link_down->get_latency() == lat);
  }

  SECTION("turn on/off")
  {
    link->turn_off();
    REQUIRE(not link_up->is_on());
    REQUIRE(not link_down->is_on());
    link->turn_on();
    REQUIRE(link_up->is_on());
    REQUIRE(link_down->is_on());
  }

  SECTION("concurrency_limit")
  {
    link->set_concurrency_limit(3);
    REQUIRE(link_up->get_impl()->get_constraint()->get_concurrency_limit() == 3);
    REQUIRE(link_down->get_impl()->get_constraint()->get_concurrency_limit() == 3);
  }
}