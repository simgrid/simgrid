/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/StarZone.hpp"
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
  simgrid::s4u::Engine e;

public:
  explicit EngineWrapper(std::string name) : argv(&name[0]), e(&argc, &argv) {}
};
} // namespace

TEST_CASE("kernel::routing::StarZone: Creating Zone", "[creation]")
{
  EngineWrapper e("test");

  REQUIRE(simgrid::s4u::create_star_zone("test"));
}

TEST_CASE("kernel::routing::StarZone: Adding routes (hosts): exception", "")
{
  EngineWrapper e("test");
  auto* zone      = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint1 = new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::Host);
  auto* netpoint2 = new simgrid::kernel::routing::NetPoint("netpoint2", simgrid::kernel::routing::NetPoint::Type::Host);

  SECTION("src and dst: nullptr")
  {
    REQUIRE_THROWS_AS(zone->add_route(nullptr, nullptr, nullptr, nullptr, {}, false), std::invalid_argument);
  }

  SECTION("src: nullptr and symmetrical: true")
  {
    REQUIRE_THROWS_AS(zone->add_route(nullptr, netpoint2, nullptr, nullptr, {}, true), std::invalid_argument);
  }

  SECTION("src and dst: not nullptr")
  {
    REQUIRE_THROWS_AS(zone->add_route(netpoint1, netpoint2, nullptr, nullptr, {}, false), std::invalid_argument);
  }
}

TEST_CASE("kernel::routing::StarZone: Adding routes (netzones): exception", "")
{
  EngineWrapper e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint1 =
      new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::NetZone);
  auto* netpoint2 =
      new simgrid::kernel::routing::NetPoint("netpoint2", simgrid::kernel::routing::NetPoint::Type::NetZone);

  SECTION("src: is a netzone and gw_src: nullptr")
  {
    REQUIRE_THROWS_AS(zone->add_route(netpoint1, nullptr, nullptr, nullptr, {}, false), std::invalid_argument);
  }

  SECTION("src: is a netzone and gw_src: is a netzone")
  {
    REQUIRE_THROWS_AS(zone->add_route(netpoint1, nullptr, netpoint2, nullptr, {}, false), std::invalid_argument);
  }

  SECTION("dst: is a netzone and gw_dst: nullptr")
  {
    REQUIRE_THROWS_AS(zone->add_route(nullptr, netpoint2, nullptr, nullptr, {}, false), std::invalid_argument);
  }

  SECTION("dst: is a netzone and gw_dst: is a netzone")
  {
    REQUIRE_THROWS_AS(zone->add_route(nullptr, netpoint2, nullptr, netpoint1, {}, false), std::invalid_argument);
  }
}

// One day we may be able to test contracts and asserts with catch2
// https://github.com/catchorg/Catch2/issues/853
TEST_CASE("kernel::routing::StarZone: Get routes: assert", "[.][assert]")
{
  /* workaround to initialize things, they must be done in this particular order */
  EngineWrapper e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();

  const auto* host1 = zone->create_host("netpoint1", {100});
  const auto* host2 = zone->create_host("netpoint2", {100});
  std::vector<simgrid::kernel::resource::LinkImpl*> links;
  links.push_back(zone->create_link("link1", {100})->get_impl());
  std::vector<simgrid::kernel::resource::LinkImpl*> links2;
  links2.push_back(zone->create_link("link2", {100})->get_impl());

  SECTION("Get route: no UP link")
  {
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(nullptr, host2->get_netpoint(), nullptr, nullptr, links2, false);
    double lat;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(host2->get_netpoint(), host1->get_netpoint(), &route, &lat);
  }

  SECTION("Get route: no DOWN link")
  {
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, false);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    double lat;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(host2->get_netpoint(), host1->get_netpoint(), &route, &lat);
  }
}

TEST_CASE("kernel::routing::StarZone: Adding routes (hosts): valid", "")
{
  EngineWrapper e("test");
  auto* zone     = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint = new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::Host);

  SECTION("Source set, destination nullptr, symmetrical true")
  {
    zone->add_route(netpoint, nullptr, nullptr, nullptr, {}, true);
  }

  SECTION("Source nullptr, destination set, symmetrical false")
  {
    zone->add_route(nullptr, netpoint, nullptr, nullptr, {}, false);
  }

  SECTION("Source set, destination nullptr, symmetrical false")
  {
    zone->add_route(netpoint, nullptr, nullptr, nullptr, {}, false);
  }

  SECTION("Source == destination") { zone->add_route(netpoint, netpoint, nullptr, nullptr, {}, false); }
}

TEST_CASE("kernel::routing::StarZone: Adding routes (netzones): valid", "")
{
  EngineWrapper e("test");
  auto* zone     = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint = new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::Host);
  auto* gw       = new simgrid::kernel::routing::NetPoint("gw1", simgrid::kernel::routing::NetPoint::Type::Router);

  SECTION("src: is a netzone, src_gw: is a router") { zone->add_route(netpoint, nullptr, gw, nullptr, {}, true); }

  SECTION("dst: is a netzone, dst_gw: is a router") { zone->add_route(nullptr, netpoint, nullptr, gw, {}, false); }
}

TEST_CASE("kernel::routing::StarZone: Get routes (hosts)", "")
{
  /* workaround to initialize things, they must be done in this particular order */
  EngineWrapper e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();

  const auto* host1 = zone->create_host("netpoint1", {100});
  const auto* host2 = zone->create_host("netpoint2", {100});

  SECTION("Get route: no shared link")
  {
    std::vector<simgrid::kernel::resource::LinkImpl*> links;
    links.push_back(zone->create_link("link1", {100})->set_latency(10)->get_impl());
    std::vector<simgrid::kernel::resource::LinkImpl*> links2;
    links2.push_back(zone->create_link("link2", {200})->set_latency(20)->get_impl());
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(host1->get_netpoint(), host2->get_netpoint(), &route, &lat);
    REQUIRE(lat == 30);
    REQUIRE(route.gw_src == nullptr);
    REQUIRE(route.gw_dst == nullptr);
    REQUIRE(route.link_list.size() == 2);
    REQUIRE(route.link_list[0]->get_name() == "link1");
    REQUIRE(route.link_list[1]->get_name() == "link2");
  }

  SECTION("Get route: shared link(backbone)")
  {
    auto* backbone = zone->create_link("backbone", {1000})->set_latency(100)->get_impl();
    std::vector<simgrid::kernel::resource::LinkImpl*> links;
    links.push_back(zone->create_link("link1", {100})->set_latency(10)->get_impl());
    links.push_back(backbone);
    std::vector<simgrid::kernel::resource::LinkImpl*> links2;
    links2.push_back(zone->create_link("link2", {200})->set_latency(20)->get_impl());
    links2.push_back(backbone);

    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(host1->get_netpoint(), host2->get_netpoint(), &route, &lat);
    REQUIRE(lat == 130);
    REQUIRE(route.link_list.size() == 3);
    REQUIRE(route.link_list[0]->get_name() == "link1");
    REQUIRE(route.link_list[1]->get_name() == "backbone");
    REQUIRE(route.link_list[2]->get_name() == "link2");
  }

  SECTION("Get route: loopback")
  {
    auto* backbone = zone->create_link("backbone", {1000})->set_latency(100)->get_impl();
    std::vector<simgrid::kernel::resource::LinkImpl*> links;
    links.push_back(zone->create_link("link1", {100})->set_latency(10)->get_impl());
    links.push_back(backbone);

    zone->add_route(host1->get_netpoint(), host1->get_netpoint(), nullptr, nullptr, links, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(host1->get_netpoint(), host1->get_netpoint(), &route, &lat);
    REQUIRE(lat == 110);
    REQUIRE(route.link_list.size() == 2);
    REQUIRE(route.link_list[0]->get_name() == "link1");
    REQUIRE(route.link_list[1]->get_name() == "backbone");
  }
}

TEST_CASE("kernel::routing::StarZone: Get routes (netzones)", "")
{
  /* workaround to initialize things, they must be done in this particular order */
  EngineWrapper e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();

  auto* subzone1 =
      (new simgrid::kernel::routing::NetPoint("subzone1", simgrid::kernel::routing::NetPoint::Type::NetZone))
          ->set_englobing_zone(zone);
  auto* subzone2 =
      (new simgrid::kernel::routing::NetPoint("subzone2", simgrid::kernel::routing::NetPoint::Type::NetZone))
          ->set_englobing_zone(zone);
  auto* router1 = new simgrid::kernel::routing::NetPoint("router1", simgrid::kernel::routing::NetPoint::Type::Router);
  auto* router2 = new simgrid::kernel::routing::NetPoint("router2", simgrid::kernel::routing::NetPoint::Type::Router);

  SECTION("Get route: netzone")
  {
    std::vector<simgrid::kernel::resource::LinkImpl*> links;
    links.push_back(zone->create_link("link1", {100})->set_latency(10)->get_impl());
    std::vector<simgrid::kernel::resource::LinkImpl*> links2;
    links2.push_back(zone->create_link("link2", {200})->set_latency(20)->get_impl());
    zone->add_route(subzone1, nullptr, router1, nullptr, links, true);
    zone->add_route(subzone2, nullptr, router2, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::RouteCreationArgs route;
    zone->get_local_route(subzone1, subzone2, &route, &lat);
    REQUIRE(lat == 30);
    REQUIRE(route.gw_src == router1);
    REQUIRE(route.gw_dst == router2);
    REQUIRE(route.link_list.size() == 2);
    REQUIRE(route.link_list[0]->get_name() == "link1");
    REQUIRE(route.link_list[1]->get_name() == "link2");
  }
}
