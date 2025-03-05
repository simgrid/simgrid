/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

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
static sg4::NetZone* create_node(sg4::NetZone* root, const std::string& node_name, const int nb_cpu)
{
  auto* node = root->add_netzone_star(node_name);

  /* create all hosts and connect them to outside world */
  for (int i = 0; i < nb_cpu; i++) {
    const auto& cpuname  = node_name + "_cpu-" + int_string(i);
    const auto& linkname = "link_" + cpuname;

    const sg4::Host* host = node->add_host(cpuname, 1e9);
    const sg4::Link* l    = node->add_split_duplex_link(linkname, BW_CPU)->set_latency(LAT_CPU);
    node->add_route(host, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }

  return node;
}

/**
 *
 * This function creates one super-node made of nb_nodes nodes with nb_cpu CPUs.
 */
static sg4::NetZone* create_supernode(sg4::NetZone* root, const std::string& supernode_name, const int nb_nodes,
                                      const int nb_cpu)
{
  auto* supernode = root->add_netzone_star(supernode_name);

  /* create all nodes and connect them to outside world */
  for (int i = 0; i < nb_nodes; i++) {
    const auto& node_name = supernode_name + "_node-" + int_string(i);
    const auto& linkname  = "link_" + node_name;

    sg4::NetZone* node = create_node(supernode, node_name, nb_cpu);
    node->set_gateway(node->add_router("router_" + node_name));
    node->seal();

    const sg4::Link* l = supernode->add_split_duplex_link(linkname, BW_NODE)->set_latency(LAT_NODE);
    supernode->add_route(node, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }
  return supernode;
}

/**
 *
 * This function creates one cluster of nb_supernodes super-nodes made of nb_nodes nodes with nb_cpu CPUs.
 */
static sg4::NetZone* create_cluster(sg4::Engine& e, const std::string& cluster_name, const int nb_supernodes,
                                    const int nb_nodes, const int nb_cpu)
{
  auto* cluster = e.get_netzone_root()->add_netzone_star(cluster_name);

  /* create all supernodes and connect them to outside world */
  for (int i = 0; i < nb_supernodes; i++) {
    const auto& supernode_name = cluster_name + "_supernode-" + int_string(i);
    const auto& linkname       = "link_" + supernode_name;

    sg4::NetZone* supernode = create_supernode(cluster, supernode_name, nb_nodes, nb_cpu);
    supernode->set_gateway(supernode->add_router("router_" + supernode_name));
    supernode->seal();

    const sg4::Link* l = cluster->add_split_duplex_link(linkname, BW_NETWORK)->set_latency(LAT_NETWORK);
    cluster->add_route(supernode, nullptr, {{l, sg4::LinkInRoute::Direction::UP}}, true);
  }
  return cluster;
}

extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e)
{
  create_cluster(e, "cluster", 4, 6, 2)->seal();
}
