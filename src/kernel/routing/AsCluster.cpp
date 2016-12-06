/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/AsCluster.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnexion with the rest of the world. */

namespace simgrid {
namespace kernel {
namespace routing {
AsCluster::AsCluster(As* father, const char* name) : AsImpl(father, name)
{
}

void AsCluster::getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t route, double* lat)
{
  XBT_VERB("cluster getLocalRoute from '%s'[%d] to '%s'[%d]", src->name().c_str(), src->id(), dst->name().c_str(),
           dst->id());
  xbt_assert(!privateLinks_.empty(),
             "Cluster routing: no links attached to the source node - did you use host_link tag?");

  if (! src->isRouter()) {    // No specific link for router

    if((src->id() == dst->id()) && hasLoopback_ ){
      s_surf_parsing_link_up_down_t info = privateLinks_.at(src->id() * linkCountPerNode_);
      route->link_list->push_back(info.linkUp);
      if (lat)
        *lat += info.linkUp->latency();
      return;
    }

    if (hasLimiter_){          // limiter for sender
      s_surf_parsing_link_up_down_t info = privateLinks_.at(src->id() * linkCountPerNode_ + (hasLoopback_ ? 1 : 0));
      route->link_list->push_back(info.linkUp);
    }

    s_surf_parsing_link_up_down_t info =
        privateLinks_.at(src->id() * linkCountPerNode_ + (hasLoopback_ ? 1 : 0) + (hasLimiter_ ? 1 : 0));
    if (info.linkUp) {         // link up
      route->link_list->push_back(info.linkUp);
      if (lat)
        *lat += info.linkUp->latency();
    }

  }

  if (backbone_) {
    route->link_list->push_back(backbone_);
    if (lat)
      *lat += backbone_->latency();
  }

  if (! dst->isRouter()) {    // No specific link for router
    s_surf_parsing_link_up_down_t info = privateLinks_.at(dst->id() * linkCountPerNode_ + hasLoopback_ + hasLimiter_);

    if (info.linkDown) {       // link down
      route->link_list->push_back(info.linkDown);
      if (lat)
        *lat += info.linkDown->latency();
    }
    if (hasLimiter_){          // limiter for receiver
        info = privateLinks_.at(dst->id() * linkCountPerNode_ + hasLoopback_);
        route->link_list->push_back(info.linkUp);
    }
  }
}

void AsCluster::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  xbt_node_t current, previous, backboneNode = nullptr;
  s_surf_parsing_link_up_down_t info;

  xbt_assert(router_,"Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  xbt_node_t routerNode = new_xbt_graph_node(graph, router_->name().c_str(), nodes);

  if(backbone_) {
    const char *link_nameR = backbone_->getName();
    backboneNode = new_xbt_graph_node(graph, link_nameR, nodes);

    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (auto src: vertices_){
    if (! src->isRouter()) {
      previous = new_xbt_graph_node(graph, src->name().c_str(), nodes);

      info = privateLinks_.at(src->id());

      if (info.linkUp) {     // link up
        const char *link_name = static_cast<simgrid::surf::Resource*>(info.linkUp)->getName();
        current = new_xbt_graph_node(graph, link_name, nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }

      if (info.linkDown) {    // link down
        const char *link_name = static_cast<simgrid::surf::Resource*>(
          info.linkDown)->getName();
        current = new_xbt_graph_node(graph, link_name, nodes);
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

void AsCluster::create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int , int position){
  s_surf_parsing_link_up_down_t info;
  char* link_id = bprintf("%s_link_%d", cluster->id, id);

  s_sg_platf_link_cbarg_t link;
  memset(&link, 0, sizeof(link));
  link.id = link_id;
  link.bandwidth = cluster->bw;
  link.latency = cluster->lat;
  link.policy = cluster->sharing_policy;
  sg_platf_new_link(&link);

  if (link.policy == SURF_LINK_FULLDUPLEX) {
    char *tmp_link = bprintf("%s_UP", link_id);
    info.linkUp = Link::byName(tmp_link);
    xbt_free(tmp_link);
    tmp_link = bprintf("%s_DOWN", link_id);
    info.linkDown = Link::byName(tmp_link);
    xbt_free(tmp_link);
  } else {
    info.linkUp = Link::byName(link_id);
    info.linkDown = info.linkUp;
  }
  privateLinks_.insert({position, info});
  xbt_free(link_id);
}

}}}
