/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

/**
 * @brief Create a new cluster netzone inside the root netzone
 *
 * This function creates the cluster, adding the hosts and links properly.
 *
 * @param root Root netzone
 * @param cluster_suffix ID of the cluster being created
 * @param host List of hostname inside the cluster
 * @param single_link_host Hostname of "special" node
 */
static sg4::NetZone* create_cluster(const sg4::NetZone* root, const std::string& cluster_suffix,
                           const std::vector<std::string>& hosts, const std::string& single_link_host)
{
  auto* cluster = sg4::create_star_zone("cluster" + cluster_suffix)->set_parent(root);

  /* create the backbone link */
  const sg4::Link* l_bb = cluster->create_link("backbone" + cluster_suffix, "20Gbps")->set_latency("500us");

  /* create all hosts and connect them to outside world */
  for (const auto& hostname : hosts) {
    /* create host */
    const sg4::Host* host = cluster->create_host(hostname, "1Gf");
    /* create UP link */
    const sg4::Link* l_up = cluster->create_link(hostname + "_up", "1Gbps")->set_latency("100us");
    /* create DOWN link, if needed */
    const sg4::Link* l_down = l_up;
    if (hostname != single_link_host) {
      l_down = cluster->create_link(hostname + "_down", "1Gbps")->set_latency("100us");
    }
    sg4::LinkInRoute backbone{l_bb};
    sg4::LinkInRoute link_up{l_up};
    sg4::LinkInRoute link_down{l_down};

    /* add link UP and backbone for communications from the host */
    cluster->add_route(host, nullptr, {link_up, backbone}, false);
    /* add backbone and link DOWN for communications to the host */
    cluster->add_route(nullptr, host, {backbone, link_down}, false);
  }

  /* create gateway*/
  cluster->set_gateway(cluster->create_router("router" + cluster_suffix));

  cluster->seal();
  return cluster;
}

/** @brief Programmatic version of routing_cluster.xml */
extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine& e)
{
  /**
   *
   * Target platform: 2 simular but irregular clusters.
   * Nodes use full-duplex links to connect to the backbone, except one node that uses a single
   * shared link.

   *                  router1 - - - - - - link1-2 - - - - - - router2
   *       __________________________                   _________________________
   *       |                        |                   |                        |
   *       |        backbone1       |                   |      backbone2         |
   *       |________________________|                   |________________________|
   *       / /         |          \ \                   / /         |          \ \
   *l1_up / / l1_down  | l3   l2_up\ \ l2_down   l4_up / / l4_down  | l6   l5_up\ \ l5_down
   *     / /           |            \ \               / /           |            \ \
   *   host1         host3         host2           host4         host6          host5
   */

  auto* root = sg4::create_full_zone("AS0");

  /* create left cluster */
  auto* left_cluster = create_cluster(root, "1", {"host1", "host2", "host3"}, "host3");
  /* create right cluster */
  auto* right_cluster = create_cluster(root, "2", {"host4", "host5", "host6"}, "host6");

  /* connect both clusters */
  const sg4::Link* l = root->create_link("link1-2", "20Gbps")->set_latency("500us");
  sg4::LinkInRoute link{l};
  root->add_route(left_cluster, right_cluster, {link});

  root->seal();
}
