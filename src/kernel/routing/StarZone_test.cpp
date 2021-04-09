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

TEST_CASE("kernel::routing::StarZone: Creating Zone", "[creation]")
{
  int argc           = 1;
  const char* argv[] = {"test"};

  simgrid::s4u::Engine e(&argc, const_cast<char**>(argv));

  REQUIRE(simgrid::s4u::create_star_zone("test"));
}

// One day we may be able to test contracts and asserts with catch2
// https://github.com/catchorg/Catch2/issues/853
TEST_CASE("kernel::routing::StarZone: Adding routes: assert", "[.][assert]")
{
  int argc           = 1;
  const char* argv[] = {"test"};
  simgrid::s4u::Engine e(&argc, const_cast<char**>(argv));
  auto* zone      = new simgrid::kernel::routing::StarZone("test");
  auto* netpoint1 = new simgrid::kernel::routing::NetPoint("netpoint1", simgrid::kernel::routing::NetPoint::Type::Host);
  auto* netpoint2 = new simgrid::kernel::routing::NetPoint("netpoint2", simgrid::kernel::routing::NetPoint::Type::Host);

  SECTION("Source and destination hosts: nullptr") { zone->add_route(nullptr, nullptr, nullptr, nullptr, {}, false); }

  SECTION("Source nullptr and symmetrical true") { zone->add_route(nullptr, netpoint2, nullptr, nullptr, {}, true); }

  SECTION("Source and destination set") { zone->add_route(netpoint1, netpoint2, nullptr, nullptr, {}, false); }
}

TEST_CASE("kernel::routing::StarZone: Get routes: assert", "[.][assert]")
{
  /* workaround to initialize things, they must be done in this particular order */
  int argc           = 1;
  const char* argv[] = {"test"};
  simgrid::s4u::Engine e(&argc, const_cast<char**>(argv));
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();

  auto* host1 = zone->create_host("netpoint1", {100});
  auto* host2 = zone->create_host("netpoint2", {100});
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

TEST_CASE("kernel::routing::StarZone: Adding routes: valid", "")
{
  int argc           = 1;
  const char* argv[] = {"test"};
  simgrid::s4u::Engine e(&argc, const_cast<char**>(argv));
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

TEST_CASE("kernel::routing::StarZone: Get routes", "")
{
  /* workaround to initialize things, they must be done in this particular order */
  int argc           = 1;
  const char* argv[] = {"test"};
  simgrid::s4u::Engine e(&argc, const_cast<char**>(argv));
  auto* zone = new simgrid::kernel::routing::StarZone("test");
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();

  auto* host1 = zone->create_host("netpoint1", {100});
  auto* host2 = zone->create_host("netpoint2", {100});

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