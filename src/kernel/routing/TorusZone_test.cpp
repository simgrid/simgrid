/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/TorusZone.hpp"
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

TEST_CASE("kernel::routing::TorusZone: Creating Zone", "")
{
  using namespace simgrid::s4u;
  EngineWrapper e("test");
  ClusterCallbacks callbacks(create_host);
  REQUIRE(create_torus_zone("test", e.e.get_netzone_root(), {3, 3, 3}, callbacks, 1e9, 10,
                            simgrid::s4u::Link::SharingPolicy::SHARED));
}

TEST_CASE("kernel::routing::TorusZone: Invalid params", "")
{
  using namespace simgrid::s4u;
  EngineWrapper e("test");
  ClusterCallbacks callbacks(create_host);

  SECTION("Empty dimensions")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {}, callbacks, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("One 0 dimension")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 0, 2}, callbacks, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("Invalid bandwidth")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 2, 2}, callbacks, 0, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
  SECTION("Invalid latency")
  {
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 2, 2}, callbacks, 1e9, -10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED),
                      std::invalid_argument);
  }
}