/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <numeric>
#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

/**
 * @brief Create a new cabinet
 *
 * This function creates the cabinet, adding the hosts and links properly.
 * See figure below for more details of each cabinet
 *
 * @param root Root netzone
 * @param name Cabinet name
 * @param radicals IDs of nodes inside the cabinet
 * @return netzone,router the created netzone and its router
 */
static std::pair<sg4::NetZone*, simgrid::kernel::routing::NetPoint*>
create_cabinet(const sg4::NetZone* root, const std::string& name, const std::vector<int>& radicals)
{
  auto* cluster      = sg4::create_star_zone(name);
  std::string prefix = "griffon-";
  std::string suffix = ".nancy.grid5000.fr";
  cluster->set_parent(root);

  /* create the backbone link */
  const sg4::Link* l_bb = cluster->create_link("backbone-" + name, "1.25GBps")->seal();
  sg4::LinkInRoute backbone(l_bb);

  /* create all hosts and connect them to outside world */
  for (const auto& id : radicals) {
    std::string hostname = prefix + std::to_string(id) + suffix;
    /* create host */
    const sg4::Host* host = cluster->create_host(hostname, "286.087kf");
    /* create UP/DOWN link */
    const sg4::Link* link = cluster->create_split_duplex_link(hostname, "125MBps")->set_latency("24us")->seal();

    /* add link and backbone for communications from the host */
    cluster->add_route(host->get_netpoint(), nullptr, nullptr, nullptr,
                       {{link, sg4::LinkInRoute::Direction::UP}, backbone}, true);
  }

  /* create router */
  auto* router = cluster->create_router(prefix + name + "-router" + suffix);

  cluster->seal();
  return std::make_pair(cluster, router);
}

/** @brief Programmatic version of griffon.xml */
extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine& /*e*/)
{
  /**
   * C++ version of griffon.xml
   * Old Grid5000 cluster (not available anymore): 3 cabinets containing homogeneous nodes connected through a backbone
   *                                  1.25GBps shared link
   *                          ___________________________________
   *          1              /                |                  \
   *                        /                 |                   \
   *                       /                  |                    \
   *     ________________ /             ______|__________           \_________________
   *     |               |              |               |            |               |
   *     | cab1 router   |              | cab2 router   |            | cab3 router   |
   *     |_______________|              |_______________|            |_______________|
   *     ++++++++++++++++               ++++++++++++++++             ++++++++++++++++++  <-- 1.25 backbone
   *     / /   | |    \ \              / /    | |    \ \             / /     | |     \ \
   *    / /    | |     \ \            / /     | |     \ \           / /      | |      \ \ <-- 125Mbps links
   *   / /     | |      \ \          / /      | |      \ \         / /       | |       \ \
   * host1     ...      hostN      host1      ...      hostM      host1      ...       hostQ
   */

  auto* root = sg4::create_star_zone("AS_griffon");
  sg4::NetZone* cab_zone;
  simgrid::kernel::routing::NetPoint* router;

  /* create top link */
  const sg4::Link* l_bb = root->create_link("backbone", "1.25GBps")->set_latency("24us")->seal();
  sg4::LinkInRoute backbone{l_bb};

  /* create cabinet1 */
  std::vector<int> rad(32);
  std::iota(rad.begin(), rad.end(), 1); // 1-29,58,59,60
  rad[rad.size() - 1]        = 60;
  rad[rad.size() - 2]        = 59;
  rad[rad.size() - 3]        = 58;
  std::tie(cab_zone, router) = create_cabinet(root, "cabinet1", rad);
  root->add_route(cab_zone->get_netpoint(), nullptr, router, nullptr, {backbone});

  /* create cabinet2 */
  rad.resize(28);
  std::iota(rad.begin(), rad.end(), 30); // 30-57
  std::tie(cab_zone, router) = create_cabinet(root, "cabinet2", rad);
  root->add_route(cab_zone->get_netpoint(), nullptr, router, nullptr, {backbone});

  /* create cabinet3 */
  rad.resize(32);
  std::iota(rad.begin(), rad.end(), 61); // 61-92
  std::tie(cab_zone, router) = create_cabinet(root, "cabinet3", rad);
  root->add_route(cab_zone->get_netpoint(), nullptr, router, nullptr, {backbone});

  root->seal();
}
