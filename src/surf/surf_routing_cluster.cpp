/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_cluster.hpp"
#include "surf_routing_private.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");
}

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnexion with the rest of the world.
 */

AS_t model_cluster_create(void)
{
  return new AsCluster();
}

/* Creation routing model functions */
AsCluster::AsCluster() : AsNone()
{
}

/* Business methods */
void AsCluster::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t route, double *lat)
{
  s_surf_parsing_link_up_down_t info;
  XBT_VERB("cluster_get_route_and_latency from '%s'[%d] to '%s'[%d]",
            src->p_name, src->m_id, dst->p_name, dst->m_id);

  if (src->p_rcType != SURF_NETWORK_ELEMENT_ROUTER) {    // No specific link for router
    info = xbt_dynar_get_as(p_linkUpDownList, src->m_id, s_surf_parsing_link_up_down_t);

    if((src->m_id == dst->m_id) && info.loopback_link  ){
      xbt_dynar_push_as(route->link_list, void *, info.loopback_link);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(info.loopback_link))->getLatency();
      return;
    }


    if (info.limiter_link)          // limiter for sender
      xbt_dynar_push_as(route->link_list, void *, info.limiter_link);

    if (info.link_up) {         // link up
      xbt_dynar_push_as(route->link_list, void *, info.link_up);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(info.link_up))->getLatency();
    }
  }

  if (p_backbone) {
    xbt_dynar_push_as(route->link_list, void *, static_cast<ResourcePtr>(p_backbone));
    if (lat)
      *lat += p_backbone->getLatency();
  }

  if (dst->p_rcType != SURF_NETWORK_ELEMENT_ROUTER) {    // No specific link for router
    info =
        xbt_dynar_get_as(p_linkUpDownList, dst->m_id, s_surf_parsing_link_up_down_t);
    if (info.link_down) {       // link down
      xbt_dynar_push_as(route->link_list, void *, info.link_down);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(info.link_down))->getLatency();
    }

    if (info.limiter_link)          // limiter for receiver
      xbt_dynar_push_as(route->link_list, void *, info.limiter_link);

  }
}

void AsCluster::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  int isrc;
  int table_size = xbt_dynar_length(p_indexNetworkElm);

  RoutingEdgePtr src;
  xbt_node_t current, previous, backboneNode = NULL, routerNode;
  s_surf_parsing_link_up_down_t info;

  xbt_assert(p_router,"Malformed cluster");

  /* create the router */
  char *link_name = p_router->p_name;
  routerNode = new_xbt_graph_node(graph, link_name, nodes);

  if(p_backbone) {
    const char *link_nameR = p_backbone->m_name;
    backboneNode = new_xbt_graph_node(graph, link_nameR, nodes);

    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (isrc = 0; isrc < table_size; isrc++) {
    src = xbt_dynar_get_as(p_indexNetworkElm, isrc, RoutingEdgePtr);

    if (src->p_rcType != SURF_NETWORK_ELEMENT_ROUTER) {
      previous = new_xbt_graph_node(graph, src->p_name, nodes);

      info = xbt_dynar_get_as(p_linkUpDownList, src->m_id, s_surf_parsing_link_up_down_t);

      if (info.link_up) {     // link up

        const char *link_name = ((ResourcePtr) info.link_up)->m_name;
        current = new_xbt_graph_node(graph, link_name, nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (p_backbone) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }

      }

      if (info.link_down) {    // link down
        const char *link_name = ((ResourcePtr) info.link_down)->m_name;
        current = new_xbt_graph_node(graph, link_name, nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (p_backbone) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }
    }

  }
}

int AsCluster::parsePU(RoutingEdgePtr elm) {
  XBT_DEBUG("Load process unit \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  return xbt_dynar_length(p_indexNetworkElm)-1;
}

int AsCluster::parseAS(RoutingEdgePtr elm) {
  XBT_DEBUG("Load Autonomous system \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  return xbt_dynar_length(p_indexNetworkElm)-1;
}


