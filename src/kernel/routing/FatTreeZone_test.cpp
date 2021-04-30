/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/FatTreeZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"

namespace {
class EngineWrapper {
public:
  explicit EngineWrapper(std::string name) : argv(&name[0]), e(&argc, &argv) {}
  int argc = 1;
  char* argv;
  simgrid::s4u::Engine e;
};

std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*>
create_host(simgrid::s4u::NetZone* zone, const std::vector<unsigned int>& /*coord*/, int id)
{
  const simgrid::s4u::Host* host = zone->create_host(std::to_string(id), 1e9)->seal();
  return std::make_pair(host->get_netpoint(), nullptr);
}
} // namespace

TEST_CASE("kernel::routing::FatTreeZone: Creating Zone", "")
{
  using namespace simgrid::s4u;
  EngineWrapper e("test");
  ClusterCallbacks callbacks(create_host);
  REQUIRE(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1, 2}}, callbacks, 1e9, 10,
                              simgrid::s4u::Link::SharingPolicy::SHARED));
}

TEST_CASE("kernel::routing::FatTreeZone: Invalid params", "")
{
  using namespace simgrid::s4u;
  EngineWrapper e("test");
  ClusterCallbacks callbacks(create_host);

  SECTION("0 levels")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {0, {4, 4}, {1, 2}, {1, 2}}, callbacks, 1e9,
                                          10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Invalid down links")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4}, {1, 2}, {1, 2}}, callbacks, 1e9, 10,
                                          simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Invalid up links")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1}, {1, 2}}, callbacks, 1e9, 10,
                                          simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Invalid link count")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1}}, callbacks, 1e9, 10,
                                          simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Down links with zeroes")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 0}, {1, 2}, {1, 2}}, callbacks, 1e9,
                                          10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Up links with zeroes")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {0, 2}, {1, 2}}, callbacks, 1e9,
                                          10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Link count with zeroes")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1, 0}}, callbacks, 1e9,
                                          10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("0 bandwidth")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1, 2}}, callbacks, 0, 10,
                                          simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }

  SECTION("Negative latency")
  {
    REQUIRE_THROWS_AS(create_fatTree_zone("test", e.e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1, 2}}, callbacks, 1e9,
                                          -10, simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
}