/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp" // FIXME: RouteCreationArgs and friends

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnection with the rest of the world. */

namespace simgrid {
namespace kernel {
namespace routing {
ClusterZone::ClusterZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : NetZoneImpl(father, name, netmodel)
{
}

void ClusterZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  XBT_VERB("cluster getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());
  xbt_assert(not private_links_.empty(),
             "Cluster routing: no links attached to the source node - did you use host_link tag?");

  if ((src->id() == dst->id()) && has_loopback_) {
    if (src->is_router()) {
      XBT_WARN("Routing from a cluster private router to itself is meaningless");
    } else {
      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos(src->id()));
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->get_latency();
    }
    return;
  }

  if (not src->is_router()) { // No private link for the private router
    if (has_limiter_) {      // limiter for sender
      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos_with_loopback(src->id()));
      route->link_list.push_back(info.first);
    }

    std::pair<resource::LinkImpl*, resource::LinkImpl*> info =
        private_links_.at(node_pos_with_loopback_limiter(src->id()));
    if (info.first) { // link up
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->get_latency();
    }
  }

  if (backbone_) {
    route->link_list.push_back(backbone_);
    if (lat)
      *lat += backbone_->get_latency();
  }

  if (not dst->is_router()) { // No specific link for router

    std::pair<resource::LinkImpl*, resource::LinkImpl*> info =
        private_links_.at(node_pos_with_loopback_limiter(dst->id()));
    if (info.second) { // link down
      route->link_list.push_back(info.second);
      if (lat)
        *lat += info.second->get_latency();
    }
    if (has_limiter_) { // limiter for receiver
      info = private_links_.at(node_pos_with_loopback(dst->id()));
      route->link_list.push_back(info.first);
    }
  }
}

void ClusterZone::get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t>* nodes,
                            std::map<std::string, xbt_edge_t>* edges)
{
  xbt_assert(router_,
             "Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  xbt_node_t routerNode = new_xbt_graph_node(graph, router_->get_cname(), nodes);

  xbt_node_t backboneNode = nullptr;
  if (backbone_) {
    backboneNode = new_xbt_graph_node(graph, backbone_->get_cname(), nodes);
    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (auto const& src : get_vertices()) {
    if (not src->is_router()) {
      xbt_node_t previous = new_xbt_graph_node(graph, src->get_cname(), nodes);

      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(src->id());

      if (info.first) { // link up
        xbt_node_t current = new_xbt_graph_node(graph, info.first->get_cname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }

      if (info.second) { // link down
        xbt_node_t current = new_xbt_graph_node(graph, info.second->get_cname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }
    }
  }
}

void ClusterZone::create_links_for_node(ClusterCreationArgs* cluster, int id, int /*rank*/, unsigned int position)
{
  std::string link_id = cluster->id + "_link_" + std::to_string(id);

  LinkCreationArgs link;
  link.id        = link_id;
  link.bandwidths.push_back(cluster->bw);
  link.latency   = cluster->lat;
  link.policy    = cluster->sharing_policy;
  sg_platf_new_link(&link);

  const s4u::Link* linkUp;
  const s4u::Link* linkDown;
  if (link.policy == simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX) {
    linkUp   = s4u::Link::by_name(link_id + "_UP");
    linkDown = s4u::Link::by_name(link_id + "_DOWN");
  } else {
    linkUp   = s4u::Link::by_name(link_id);
    linkDown = linkUp;
  }
  private_links_.insert({position, {linkUp->get_impl(), linkDown->get_impl()}});
}
}
}
}
