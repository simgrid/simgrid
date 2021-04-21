/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"    // create models
#include "src/surf/xml/platf_private.hpp" // RouteCreationArgs and friends

namespace {
class EngineWrapper {
  int argc = 1;
  char* argv;

public:
  simgrid::s4u::Engine e;
  explicit EngineWrapper(std::string name) : argv(&name[0]), e(&argc, &argv)
  {
    simgrid::s4u::create_full_zone("root");
    surf_network_model_init_LegrandVelho();
    surf_cpu_model_init_Cas01();
  }
};

std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*>
create_host(simgrid::s4u::NetZone* zone, const std::vector<unsigned int>& coord, int id)
{
  simgrid::s4u::Host* host = zone->create_host(std::to_string(id), 1e9)->seal();
  return std::make_pair(host->get_netpoint(), nullptr);
}
} // namespace

TEST_CASE("kernel::routing::TorusZone: Creating Zone", "")
{
  using namespace simgrid::s4u;
  EngineWrapper e("test");
  auto create_host = [](NetZone* zone, const std::vector<unsigned int>& coord,
                        int id) -> std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*> {
    Host* host = zone->create_host(std::to_string(id), 1e9)->seal();
    return std::make_pair(host->get_netpoint(), nullptr);
  };
  REQUIRE(create_torus_zone("test", e.e.get_netzone_root(), {3, 3, 3}, 1e9, 10,
                            simgrid::s4u::Link::SharingPolicy::SHARED, create_host));
}

TEST_CASE("kernel::routing::TorusZone: Invalid params", "")
{
  SECTION("Empty dimensions")
  {
    using namespace simgrid::s4u;
    EngineWrapper e("test");
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {}, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED, create_host),
                      std::invalid_argument);
  }
  SECTION("One 0 dimension")
  {
    using namespace simgrid::s4u;
    EngineWrapper e("test");
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 0, 2}, 1e9, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED, create_host),
                      std::invalid_argument);
  }
  SECTION("Invalid bandwidth")
  {
    using namespace simgrid::s4u;
    EngineWrapper e("test");
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 2, 2}, 0, 10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED, create_host),
                      std::invalid_argument);
  }
  SECTION("Invalid latency")
  {
    using namespace simgrid::s4u;
    EngineWrapper e("test");
    REQUIRE_THROWS_AS(create_torus_zone("test", e.e.get_netzone_root(), {3, 2, 2}, 1e9, -10,
                                        simgrid::s4u::Link::SharingPolicy::SHARED, create_host),
                      std::invalid_argument);
  }
}