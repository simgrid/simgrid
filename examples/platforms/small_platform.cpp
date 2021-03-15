/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

/**
 * @brief Create the hosts in the platform
 *
 * @param zone Zone to insert the hosts
 */
static void create_sp_hosts(sg4::NetZone* zone)
{
  zone->create_host("Tremblay", {"98.095Mf"})->seal();
  zone->create_host("Jupiter", {"76.296Mf"})->seal();
  zone->create_host("Fafard", {"76.296Mf"})->seal();
  zone->create_host("Ginette", {"48.492Mf"})->seal();
  zone->create_host("Bourassa", {"48.492Mf"})->seal();
  zone->create_host("Jacquelin", {"137.333Mf"})->seal();
  zone->create_host("Boivin", {"98.095Mf"})->seal();
}

/**
 * @brief Create the links in the platform
 *
 * They'll be used later in the routes.
 *
 * @param zone Netzone
 */
static void create_sp_links(sg4::NetZone* zone)
{
  zone->create_link("6", {"41.279125MBps"})->set_latency("59.904us")->seal();
  zone->create_link("3", {"34.285625MBps"})->set_latency("514.433us")->seal();
  zone->create_link("7", {"11.618875MBps"})->set_latency("189.98us")->seal();
  zone->create_link("9", {"7.20975MBps"})->set_latency("1.461517ms")->seal();
  zone->create_link("2", {"118.6825MBps"})->set_latency("136.931us")->seal();
  zone->create_link("8", {"8.158MBps"})->set_latency("270.544us")->seal();
  zone->create_link("1", {"34.285625MBps"})->set_latency("514.433us")->seal();
  zone->create_link("4", {"10.099625MBps"})->set_latency("479.78us")->seal();
  zone->create_link("0", {"41.279125MBps"})->set_latency("59.904us")->seal();
  zone->create_link("5", {"27.94625MBps"})->set_latency("278.066us")->seal();
  zone->create_link("145", {"2.583375MBps"})->set_latency("410.463us")->seal();
  zone->create_link("10", {"34.285625MBps"})->set_latency("514.433us")->seal();
  zone->create_link("11", {"118.6825MBps"})->set_latency("136.931us")->seal();
  zone->create_link("16", {"34.285625MBps"})->set_latency("514.433us")->seal();
  zone->create_link("17", {"118.6825MBps"})->set_latency("136.931us")->seal();
  zone->create_link("44", {"10.314625MBps"})->set_latency("6.932556ms")->seal();
  zone->create_link("47", {"10.314625MBps"})->set_latency("6.932556ms")->seal();
  zone->create_link("54", {"15.376875MBps"})->set_latency("35.083019ms")->seal();
  zone->create_link("56", {"21.41475MBps"})->set_latency("29.5890617ms")->seal();
  zone->create_link("59", {"11.845375MBps"})->set_latency("370.788us")->seal();
  zone->create_link("78", {"27.94625MBps"})->set_latency("278.066us")->seal();
  zone->create_link("79", {"8.42725MBps"})->set_latency("156.056us")->seal();
  zone->create_link("80", {"15.376875MBps"})->set_latency("35.083019ms")->seal();

  /* single FATPIPE links for loopback */
  zone->create_link("loopback", {"498MBps"})
      ->set_latency("15us")
      ->set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)
      ->seal();
}

/**
 * @brief Auxiliary function to create a single route
 *
 * It translates the parameters from string to proper API
 * @param e S4U Engine
 * @param src Source hostname
 * @param dst Destination hostname
 * @param links List of links to use in this route
 */
static void create_single_route(const sg4::Engine& e, std::string const& src, std::string const& dst,
                                std::vector<std::string> const& links)
{
  std::vector<sg4::Link*> link_list;
  for (auto& link : links) {
    link_list.push_back(e.link_by_name(link));
  }
  sg4::NetZone* zone = e.get_netzone_root();
  zone->add_route(e.netpoint_by_name(src), e.netpoint_by_name(dst), nullptr, nullptr, link_list);
}

/** @brief Creates the routes in the platform */
static void create_sp_routes(const sg4::Engine& e)
{
  auto nodes = std::vector<std::string>{"Tremblay", "Jupiter", "Fafard", "Ginette", "Bourassa"};
  for (const auto& name : nodes) {
    create_single_route(e, name, name, {"loopback"});
  }
  create_single_route(e, "Tremblay", "Jupiter", {"9"});
  create_single_route(e, "Tremblay", "Fafard", {"4", "3", "2", "0", "1", "8"});
  create_single_route(e, "Tremblay", "Ginette", {"4", "3", "5"});
  create_single_route(e, "Tremblay", "Bourassa", {"4", "3", "2", "0", "1", "6", "7"});
  create_single_route(e, "Jupiter", "Fafard", {"9", "4", "3", "2", "0", "1", "8"});
  create_single_route(e, "Jupiter", "Bourassa", {"9", "4", "3", "2", "0", "1", "6", "7"});
  create_single_route(e, "Fafard", "Ginette", {"8", "1", "0", "2", "5"});
  create_single_route(e, "Jupiter", "Jacquelin", {"145"});
  create_single_route(e, "Jupiter", "Boivin", {"47"});
  create_single_route(e, "Jupiter", "Ginette", {"9", "4", "3", "5"});
  create_single_route(e, "Fafard", "Bourassa", {"8", "6", "7"});
  create_single_route(e, "Ginette", "Bourassa", {"5", "2", "0", "1", "6", "7"});
  create_single_route(e, "Ginette", "Jacquelin", {"145"});
  create_single_route(e, "Ginette", "Boivin", {"47"});
  create_single_route(e, "Bourassa", "Jacquelin", {"145"});
  create_single_route(e, "Bourassa", "Boivin", {"47"});
  create_single_route(e, "Jacquelin", "Boivin", {"145", "59", "56", "54", "17", "16", "10", "11", "44", "47"});
  create_single_route(e, "Jacquelin", "Fafard", {"145", "59", "56", "54", "17", "16", "10", "6", "9", "79", "78"});
  create_single_route(e, "Jacquelin", "Tremblay", {"145", "59", "56", "54", "2", "3"});
  create_single_route(e, "Boivin", "Tremblay", {"47", "44", "11", "10", "16", "0", "3"});
  create_single_route(e, "Boivin", "Fafard", {"47", "44", "11", "6", "9", "79", "78", "80"});
}

/**
 * @brief Programmatic version of small_platform.xml
 *
 * Possible not the best example since we need to describe each component manually,
 * but it illustrates the new API
 */
extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine& e)
{
  auto* root_zone = sg4::create_full_zone("zone0");

  /* create hosts */
  create_sp_hosts(root_zone);
  /* create links */
  create_sp_links(root_zone);
  /* create routes */
  create_sp_routes(e);
  /* finally seal the root zone */
  root_zone->seal();
}