/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/AsCluster.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnexion with the rest of the world. */

namespace simgrid {
namespace surf {
  AsCluster::AsCluster(const char*name)
    : AsImpl(name)
  {}
  AsCluster::~AsCluster()
  {
    xbt_dynar_free(&privateLinks_);
  }

void AsCluster::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t route, double *lat)
{
  s_surf_parsing_link_up_down_t info;
  XBT_VERB("cluster_get_route_and_latency from '%s'[%d] to '%s'[%d]",
            src->name(), src->id(), dst->name(), dst->id());

  if (! src->isRouter()) {    // No specific link for router

    if((src->id() == dst->id()) && hasLoopback_ ){
      info = xbt_dynar_get_as(privateLinks_, src->id() * linkCountPerNode_, s_surf_parsing_link_up_down_t);
      route->link_list->push_back(info.linkUp);
      if (lat)
        *lat += info.linkUp->getLatency();
      return;
    }


    if (hasLimiter_){          // limiter for sender
      info = xbt_dynar_get_as(privateLinks_, src->id() * linkCountPerNode_ + (hasLoopback_?1:0), s_surf_parsing_link_up_down_t);
      route->link_list->push_back((Link*)info.linkUp);
    }

    info = xbt_dynar_get_as(privateLinks_, src->id() * linkCountPerNode_ + (hasLoopback_?1:0) + (hasLimiter_?1:0), s_surf_parsing_link_up_down_t);
    if (info.linkUp) {         // link up
      route->link_list->push_back(info.linkUp);
      if (lat)
        *lat += info.linkUp->getLatency();
    }

  }

  if (backbone_) {
    route->link_list->push_back(backbone_);
    if (lat)
      *lat += backbone_->getLatency();
  }

  if (! dst->isRouter()) {    // No specific link for router
    info = xbt_dynar_get_as(privateLinks_, dst->id() * linkCountPerNode_ + hasLoopback_ + hasLimiter_, s_surf_parsing_link_up_down_t);

    if (info.linkDown) {       // link down
      route->link_list->push_back(info.linkDown);
      if (lat)
        *lat += info.linkDown->getLatency();
    }
    if (hasLimiter_){          // limiter for receiver
        info = xbt_dynar_get_as(privateLinks_, dst->id() * linkCountPerNode_ + hasLoopback_, s_surf_parsing_link_up_down_t);
        route->link_list->push_back(info.linkUp);
    }
  }
}

void AsCluster::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  int isrc;
  int table_size = xbt_dynar_length(vertices_);

  NetCard *src;
  xbt_node_t current, previous, backboneNode = nullptr, routerNode;
  s_surf_parsing_link_up_down_t info;

  xbt_assert(router_,"Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  char *link_name = router_->name();
  routerNode = new_xbt_graph_node(graph, link_name, nodes);

  if(backbone_) {
    const char *link_nameR = backbone_->getName();
    backboneNode = new_xbt_graph_node(graph, link_nameR, nodes);

    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (isrc = 0; isrc < table_size; isrc++) {
    src = xbt_dynar_get_as(vertices_, isrc, NetCard*);

    if (! src->isRouter()) {
      previous = new_xbt_graph_node(graph, src->name(), nodes);

      info = xbt_dynar_get_as(privateLinks_, src->id(), s_surf_parsing_link_up_down_t);

      if (info.linkUp) {     // link up

        const char *link_name = static_cast<simgrid::surf::Resource*>(
          info.linkUp)->getName();
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
  xbt_dynar_set(privateLinks_, position, &info);
  xbt_free(link_id);
}

}
}
