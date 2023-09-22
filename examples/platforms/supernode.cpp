/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

constexpr double BW_CPU  = 1e12;
constexpr double LAT_CPU = 0;

constexpr double BW_NODE  = 1e11;
constexpr double LAT_NODE = 1e-8;

constexpr double BW_NETWORK  = 1.25e10;
constexpr double LAT_NETWORK = 1e-7;

static std::string int_string(int n)
{
  return simgrid::xbt::string_printf("%02d", n);
}

/**
 *
 * This function creates one node made of nb_cpu CPUs.
 */
static sg4::NetZone* create_node(const sg4::NetZone* root, const std::string& node_name, const int nb_cpu)
{
  auto* node = sg4::create_star_zone(node_name)->set_parent(root);

  /* create all hosts and connect them to outside world */
  for (int i = 0; i < nb_cpu; i++) {
    const auto& cpuname  = node_name + "_cpu-" + int_string(i);
    const auto& linkname = "link_" + cpuname;

    const sg4::Host* host = node->create_host(cpuname, 1e9);
    const sg4::Link* l    = node->create_split_duplex_link(linkname, BW_CPU)->set_latency(LAT_CPU);
    node->add_route(host, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }

  return node;
}

/**
 *
 * This function creates one super-node made of nb_nodes nodes with nb_cpu CPUs.
 */
static sg4::NetZone* create_supernode(const sg4::NetZone* root, const std::string& supernode_name, const int nb_nodes,
                                      const int nb_cpu)
{
  auto* supernode = sg4::create_star_zone(supernode_name)->set_parent(root);

  /* create all nodes and connect them to outside world */
  for (int i = 0; i < nb_nodes; i++) {
    const auto& node_name = supernode_name + "_node-" + int_string(i);
    const auto& linkname  = "link_" + node_name;

    sg4::NetZone* node = create_node(supernode, node_name, nb_cpu);
    node->set_gateway(node->create_router("router_" + node_name));
    node->seal();

    const sg4::Link* l = supernode->create_split_duplex_link(linkname, BW_NODE)->set_latency(LAT_NODE);
    supernode->add_route(node, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }
  return supernode;
}

/**
 *
 * This function creates one cluster of nb_supernodes super-nodes made of nb_nodes nodes with nb_cpu CPUs.
 */
static sg4::NetZone* create_cluster(const std::string& cluster_name, const int nb_supernodes, const int nb_nodes,
                                    const int nb_cpu)
{
  auto* cluster = sg4::create_star_zone(cluster_name);

  /* create all supernodes and connect them to outside world */
  for (int i = 0; i < nb_supernodes; i++) {
    const auto& supernode_name = cluster_name + "_supernode-" + int_string(i);
    const auto& linkname       = "link_" + supernode_name;

    sg4::NetZone* supernode = create_supernode(cluster, supernode_name, nb_nodes, nb_cpu);
    supernode->set_gateway(supernode->create_router("router_" + supernode_name));
    supernode->seal();

    const sg4::Link* l = cluster->create_split_duplex_link(linkname, BW_NETWORK)->set_latency(LAT_NETWORK);
    cluster->add_route(supernode, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }
  return cluster;
}

extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine&)
{
  create_cluster("cluster", 4, 6, 2)->seal();
}
