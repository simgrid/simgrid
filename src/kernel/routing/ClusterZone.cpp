/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

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
ClusterZone::ClusterZone(NetZone* father, const char* name) : NetZoneImpl(father, name)
{
}

void ClusterZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* lat)
{
  XBT_VERB("cluster getLocalRoute from '%s'[%d] to '%s'[%d]", src->cname(), src->id(), dst->cname(), dst->id());
  xbt_assert(!privateLinks_.empty(),
             "Cluster routing: no links attached to the source node - did you use host_link tag?");

  if ((src->id() == dst->id()) && hasLoopback_) {
    xbt_assert(!src->isRouter(), "Routing from a cluster private router to itself is meaningless");

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(src->id() * linkCountPerNode_);
    route->link_list->push_back(info.first);
    if (lat)
      *lat += info.first->latency();
    return;
  }

  if (!src->isRouter()) { // No private link for the private router
    if (hasLimiter_) { // limiter for sender
      std::pair<surf::LinkImpl*, surf::LinkImpl*> info =
          privateLinks_.at(src->id() * linkCountPerNode_ + (hasLoopback_ ? 1 : 0));
      route->link_list->push_back(info.first);
    }

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info =
        privateLinks_.at(src->id() * linkCountPerNode_ + (hasLoopback_ ? 1 : 0) + (hasLimiter_ ? 1 : 0));
    if (info.first) { // link up
      route->link_list->push_back(info.first);
      if (lat)
        *lat += info.first->latency();
    }
  }

  if (backbone_) {
    route->link_list->push_back(backbone_);
    if (lat)
      *lat += backbone_->latency();
  }

  if (!dst->isRouter()) { // No specific link for router

    std::pair<surf::LinkImpl*, surf::LinkImpl*> info =
        privateLinks_.at(dst->id() * linkCountPerNode_ + hasLoopback_ + hasLimiter_);
    if (info.second) { // link down
      route->link_list->push_back(info.second);
      if (lat)
        *lat += info.second->latency();
    }
    if (hasLimiter_) { // limiter for receiver
      info = privateLinks_.at(dst->id() * linkCountPerNode_ + hasLoopback_);
      route->link_list->push_back(info.first);
    }
  }
}

void ClusterZone::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  xbt_assert(router_,
             "Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  xbt_node_t routerNode = new_xbt_graph_node(graph, router_->cname(), nodes);

  xbt_node_t backboneNode = nullptr;
  if (backbone_) {
    backboneNode = new_xbt_graph_node(graph, backbone_->cname(), nodes);
    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (auto src : vertices_) {
    if (!src->isRouter()) {
      xbt_node_t previous = new_xbt_graph_node(graph, src->cname(), nodes);

      std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(src->id());

      if (info.first) { // link up
        xbt_node_t current = new_xbt_graph_node(graph, info.first->cname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }

      if (info.second) { // link down
        xbt_node_t current = new_xbt_graph_node(graph, info.second->cname(), nodes);
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

void ClusterZone::create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int /*rank*/, int position)
{
  char* link_id = bprintf("%s_link_%d", cluster->id, id);

  s_sg_platf_link_cbarg_t link;
  memset(&link, 0, sizeof(link));
  link.id        = link_id;
  link.bandwidth = cluster->bw;
  link.latency   = cluster->lat;
  link.policy    = cluster->sharing_policy;
  sg_platf_new_link(&link);

  surf::LinkImpl *linkUp;
  surf::LinkImpl *linkDown;
  if (link.policy == SURF_LINK_FULLDUPLEX) {
    char* tmp_link = bprintf("%s_UP", link_id);
    linkUp         = surf::LinkImpl::byName(tmp_link);
    xbt_free(tmp_link);
    tmp_link = bprintf("%s_DOWN", link_id);
    linkDown = surf::LinkImpl::byName(tmp_link);
    xbt_free(tmp_link);
  } else {
    linkUp   = surf::LinkImpl::byName(link_id);
    linkDown = linkUp;
  }
  privateLinks_.insert({position, {linkUp, linkDown}});
  xbt_free(link_id);
}
}
}
}
