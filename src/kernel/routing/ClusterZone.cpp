/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/ClusterZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnexion with the rest of the world. */

namespace simgrid {
namespace kernel {
namespace routing {
ClusterZone::ClusterZone(NetZone* father, std::string name) : NetZoneImpl(father, name)
{
}

void ClusterZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* lat)
{
  XBT_VERB("cluster getLocalRoute from '%s'[%u] to '%s'[%u]", src->getCname(), src->id(), dst->getCname(), dst->id());
  xbt_assert(not privateLinks_.empty(),
             "Cluster routing: no links attached to the source node - did you use host_link tag?");

  if ((src->id() == dst->id()) && hasLoopback_) {
    xbt_assert(not src->isRouter(), "Routing from a cluster private router to itself is meaningless");

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePosition(src->id()));
    route->link_list.push_back(info.first);
    if (lat)
      *lat += info.first->latency();
    return;
  }

  if (not src->isRouter()) { // No private link for the private router
    if (hasLimiter_) { // limiter for sender
      std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePositionWithLoopback(src->id()));
      route->link_list.push_back(info.first);
    }

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePositionWithLimiter(src->id()));
    if (info.first) { // link up
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->latency();
    }
  }

  if (backbone_) {
    route->link_list.push_back(backbone_);
    if (lat)
      *lat += backbone_->latency();
  }

  if (not dst->isRouter()) { // No specific link for router

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePositionWithLimiter(dst->id()));
    if (info.second) { // link down
      route->link_list.push_back(info.second);
      if (lat)
        *lat += info.second->latency();
    }
    if (hasLimiter_) { // limiter for receiver
      info = privateLinks_.at(nodePositionWithLoopback(dst->id()));
      route->link_list.push_back(info.first);
    }
  }
}

void ClusterZone::getGraph(xbt_graph_t graph, std::map<std::string, xbt_node_t>* nodes,
                           std::map<std::string, xbt_edge_t>* edges)
{
  xbt_assert(router_,
             "Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  xbt_node_t routerNode = new_xbt_graph_node(graph, router_->getCname(), nodes);

  xbt_node_t backboneNode = nullptr;
  if (backbone_) {
    backboneNode = new_xbt_graph_node(graph, backbone_->getCname(), nodes);
    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (auto const& src : getVertices()) {
    if (not src->isRouter()) {
      xbt_node_t previous = new_xbt_graph_node(graph, src->getCname(), nodes);

      std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(src->id());

      if (info.first) { // link up
        xbt_node_t current = new_xbt_graph_node(graph, info.first->getCname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }

      if (info.second) { // link down
        xbt_node_t current = new_xbt_graph_node(graph, info.second->getCname(), nodes);
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
  link.bandwidth = cluster->bw;
  link.latency   = cluster->lat;
  link.policy    = cluster->sharing_policy;
  sg_platf_new_link(&link);

  surf::LinkImpl *linkUp;
  surf::LinkImpl *linkDown;
  if (link.policy == SURF_LINK_FULLDUPLEX) {
    linkUp   = surf::LinkImpl::byName(link_id + "_UP");
    linkDown = surf::LinkImpl::byName(link_id + "_DOWN");
  } else {
    linkUp   = surf::LinkImpl::byName(link_id);
    linkDown = linkUp;
  }
  privateLinks_.insert({position, {linkUp, linkDown}});
}
}
}
}
