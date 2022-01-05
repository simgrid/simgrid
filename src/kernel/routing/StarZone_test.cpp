/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/StarZone.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

TEST_CASE("kernel::routing::StarZone: Creating Zone", "[creation]")
{
  simgrid::s4u::Engine e("test");

  REQUIRE(simgrid::s4u::create_star_zone("test"));
}

TEST_CASE("kernel::routing::StarZone: Create links: exceptions", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = simgrid::s4u::create_star_zone("test");
  SECTION("create_link: invalid bandwidth")
  {
    REQUIRE_THROWS_AS(zone->create_link("link", "speed"), std::invalid_argument);
  }

  SECTION("split-duplex create_link: invalid bandwidth")
  {
    REQUIRE_THROWS_AS(zone->create_split_duplex_link("link", "speed"), std::invalid_argument);
  }
}

TEST_CASE("kernel::routing::StarZone: Adding routes (hosts): exception", "")
{
  simgrid::s4u::Engine e("test");
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
  simgrid::s4u::Engine e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint1 =
      new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::NetZone);
  auto* netpoint2 =
      new simgrid::kernel::routing::NetPoint("netpoint2", simgrid::kernel::routing::NetPoint::Type::NetZone);
  auto zone3      = std::make_unique<simgrid::kernel::routing::StarZone>("test3");
  auto* netpoint3 = zone3->create_router("netpoint3");

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

  SECTION("issue71: gw_src isn't member of the src netzone")
  {
    REQUIRE_THROWS_AS(zone->add_route(zone->get_netpoint(), nullptr, netpoint3, nullptr, {}, false),
                      std::invalid_argument);
  }

  SECTION("issue71: gw_dst isn't member of the dst netzone")
  {
    REQUIRE_THROWS_AS(zone->add_route(nullptr, zone->get_netpoint(), nullptr, netpoint3, {}, false),
                      std::invalid_argument);
  }
}

// One day we may be able to test contracts and asserts with catch2
// https://github.com/catchorg/Catch2/issues/853
TEST_CASE("kernel::routing::StarZone: Get routes: assert", "[.][assert]")
{
  simgrid::s4u::Engine e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");

  const auto* host1 = zone->create_host("netpoint1", {100});
  const auto* host2 = zone->create_host("netpoint2", {100});
  std::vector<simgrid::s4u::LinkInRoute> links;
  links.emplace_back(zone->create_link("link1", {100}));
  std::vector<simgrid::s4u::LinkInRoute> links2;
  links2.emplace_back(zone->create_link("link2", {100}));

  SECTION("Get route: no UP link")
  {
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(nullptr, host2->get_netpoint(), nullptr, nullptr, links2, false);
    double lat;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(host2->get_netpoint(), host1->get_netpoint(), &route, &lat);
  }

  SECTION("Get route: no DOWN link")
  {
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, false);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    double lat;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(host2->get_netpoint(), host1->get_netpoint(), &route, &lat);
  }
}

TEST_CASE("kernel::routing::StarZone: Adding routes (hosts): valid", "")
{
  simgrid::s4u::Engine e("test");
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
  simgrid::s4u::Engine e("test");
  auto* zone     = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint = new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::Host);
  auto* gw       = new simgrid::kernel::routing::NetPoint("gw1", simgrid::kernel::routing::NetPoint::Type::Router);

  SECTION("src: is a netzone, src_gw: is a router") { zone->add_route(netpoint, nullptr, gw, nullptr, {}, true); }

  SECTION("dst: is a netzone, dst_gw: is a router") { zone->add_route(nullptr, netpoint, nullptr, gw, {}, false); }
}

TEST_CASE("kernel::routing::StarZone: Get routes (hosts)", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");

  const auto* host1 = zone->create_host("netpoint1", {100});
  const auto* host2 = zone->create_host("netpoint2", {100});

  SECTION("Get route: no shared link")
  {
    std::vector<simgrid::s4u::LinkInRoute> links;
    links.emplace_back(zone->create_link("link1", {100})->set_latency(10));
    std::vector<simgrid::s4u::LinkInRoute> links2;
    links2.emplace_back(zone->create_link("link2", {200})->set_latency(20));
    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(host1->get_netpoint(), host2->get_netpoint(), &route, &lat);
    REQUIRE(lat == 30);
    REQUIRE(route.gw_src_ == nullptr);
    REQUIRE(route.gw_dst_ == nullptr);
    REQUIRE(route.link_list_.size() == 2);
    REQUIRE(route.link_list_[0]->get_name() == "link1");
    REQUIRE(route.link_list_[1]->get_name() == "link2");
  }

  SECTION("Get route: shared link(backbone)")
  {
    auto* backbone = zone->create_link("backbone", {1000})->set_latency(100);
    std::vector<simgrid::s4u::LinkInRoute> links;
    links.emplace_back(zone->create_link("link1", {100})->set_latency(10));
    links.emplace_back(backbone);
    std::vector<simgrid::s4u::LinkInRoute> links2;
    links2.emplace_back(zone->create_link("link2", {200})->set_latency(20));
    links2.emplace_back(backbone);

    zone->add_route(host1->get_netpoint(), nullptr, nullptr, nullptr, links, true);
    zone->add_route(host2->get_netpoint(), nullptr, nullptr, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(host1->get_netpoint(), host2->get_netpoint(), &route, &lat);
    REQUIRE(lat == 130);
    REQUIRE(route.link_list_.size() == 3);
    REQUIRE(route.link_list_[0]->get_name() == "link1");
    REQUIRE(route.link_list_[1]->get_name() == "backbone");
    REQUIRE(route.link_list_[2]->get_name() == "link2");
  }

  SECTION("Get route: loopback")
  {
    auto* backbone = zone->create_link("backbone", {1000})->set_latency(100);
    std::vector<simgrid::s4u::LinkInRoute> links;
    links.emplace_back(zone->create_link("link1", {100})->set_latency(10));
    links.emplace_back(backbone);

    zone->add_route(host1->get_netpoint(), host1->get_netpoint(), nullptr, nullptr, links, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(host1->get_netpoint(), host1->get_netpoint(), &route, &lat);
    REQUIRE(lat == 110);
    REQUIRE(route.link_list_.size() == 2);
    REQUIRE(route.link_list_[0]->get_name() == "link1");
    REQUIRE(route.link_list_[1]->get_name() == "backbone");
  }
}

TEST_CASE("kernel::routing::StarZone: Get routes (netzones)", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = new simgrid::kernel::routing::StarZone("test");

  auto* subzone1 = new simgrid::kernel::routing::StarZone("subzone1");
  subzone1->set_parent(zone);
  auto* subzone2 = new simgrid::kernel::routing::StarZone("subzone2");
  subzone2->set_parent(zone);

  auto* router1 = subzone1->create_router("router1");
  auto* router2 = subzone2->create_router("router2");

  SECTION("Get route: netzone")
  {
    std::vector<simgrid::s4u::LinkInRoute> links;
    links.emplace_back(zone->create_link("link1", {100})->set_latency(10));
    std::vector<simgrid::s4u::LinkInRoute> links2;
    links2.emplace_back(zone->create_link("link2", {200})->set_latency(20));
    zone->add_route(subzone1->get_netpoint(), nullptr, router1, nullptr, links, true);
    zone->add_route(subzone2->get_netpoint(), nullptr, router2, nullptr, links2, true);
    zone->seal();

    double lat = 0.0;
    simgrid::kernel::routing::Route route;
    zone->get_local_route(subzone1->get_netpoint(), subzone2->get_netpoint(), &route, &lat);
    REQUIRE(lat == 30);
    REQUIRE(route.gw_src_ == router1);
    REQUIRE(route.gw_dst_ == router2);
    REQUIRE(route.link_list_.size() == 2);
    REQUIRE(route.link_list_[0]->get_name() == "link1");
    REQUIRE(route.link_list_[1]->get_name() == "link2");
  }
}

TEST_CASE("kernel::routing::StarZone: mix new routes and hosts", "")
{
  simgrid::s4u::Engine e("test");
  auto* zone = simgrid::s4u::create_star_zone("test");

  const simgrid::s4u::Link* link = zone->create_link("my_link", 1e6)->seal();
  for (int i = 0; i < 10; i++) {
    std::string cpu_name          = "CPU" + std::to_string(i);
    const simgrid::s4u::Host* cpu = zone->create_host(cpu_name, 1e9)->seal();
    REQUIRE_NOTHROW(
        zone->add_route(cpu->get_netpoint(), nullptr, nullptr, nullptr, {simgrid::s4u::LinkInRoute(link)}, true));
  }
}
