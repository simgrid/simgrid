/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_RoutedGraph.hpp"

#include <cstdlib>

#include <algorithm>

#include <xbt/dict.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <xbt/graph.h>

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_private.hpp"
#include "network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");

void routing_route_free(sg_platf_route_cbarg_t route)
{
  if (route) {
    xbt_dynar_free(&route->link_list);
    xbt_free(route);
  }
}

namespace simgrid {
namespace surf {
  
AsRoutedGraph::AsRoutedGraph(const char*name)
  : As(name)
{
}

AsRoutedGraph::~AsRoutedGraph()
{
}


}
}

/* ************************************************************************** */
/* *********************** GENERIC BUSINESS METHODS ************************* */

static const char *instr_node_name(xbt_node_t node)
{
  void *data = xbt_graph_node_get_data(node);
  char *str = (char *) data;
  return str;
}

xbt_node_t new_xbt_graph_node(xbt_graph_t graph, const char *name,
                              xbt_dict_t nodes)
{
  xbt_node_t ret = (xbt_node_t) xbt_dict_get_or_null(nodes, name);
  if (ret)
    return ret;

  ret = xbt_graph_new_node(graph, xbt_strdup(name));
  xbt_dict_set(nodes, name, ret, NULL);
  return ret;
}

xbt_edge_t new_xbt_graph_edge(xbt_graph_t graph, xbt_node_t s, xbt_node_t d,
                              xbt_dict_t edges)
{
  xbt_edge_t ret;

  const char *sn = instr_node_name(s);
  const char *dn = instr_node_name(d);
  int len = strlen(sn) + strlen(dn) + 1;
  char *name = (char *) xbt_malloc(len * sizeof(char));


  snprintf(name, len, "%s%s", sn, dn);
  ret = (xbt_edge_t) xbt_dict_get_or_null(edges, name);
  if (ret == NULL) {
    snprintf(name, len, "%s%s", dn, sn);
    ret = (xbt_edge_t) xbt_dict_get_or_null(edges, name);
  }

  if (ret == NULL) {
    ret = xbt_graph_new_edge(graph, s, d, NULL);
    xbt_dict_set(edges, name, ret, NULL);
  }
  free(name);
  return ret;
}

namespace simgrid {
namespace surf {

void AsRoutedGraph::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  int src, dst;
  int table_size = xbt_dynar_length(vertices_);


  for (src = 0; src < table_size; src++) {
    NetCard *my_src =
        xbt_dynar_get_as(vertices_, src, NetCard*);
    for (dst = 0; dst < table_size; dst++) {
      if (src == dst)
        continue;
      NetCard *my_dst =
          xbt_dynar_get_as(vertices_, dst, NetCard*);

      sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
      route->link_list = xbt_dynar_new(sizeof(Link*), NULL);

      getRouteAndLatency(my_src, my_dst, route, NULL);

      XBT_DEBUG ("get_route_and_latency %s -> %s", my_src->name(), my_dst->name());

      unsigned int cpt;
      void *link;

      xbt_node_t current, previous;
      const char *previous_name, *current_name;

      if (route->gw_src) {
        previous = new_xbt_graph_node(graph, route->gw_src->name(), nodes);
        previous_name = route->gw_src->name();
      } else {
        previous = new_xbt_graph_node(graph, my_src->name(), nodes);
        previous_name = my_src->name();
      }

      xbt_dynar_foreach(route->link_list, cpt, link) {
        const char *link_name = static_cast<simgrid::surf::Resource*>(
          link)->getName();
        current = new_xbt_graph_node(graph, link_name, nodes);
        current_name = link_name;
        new_xbt_graph_edge(graph, previous, current, edges);
        XBT_DEBUG ("  %s -> %s", previous_name, current_name);
        previous = current;
        previous_name = current_name;
      }

      if (route->gw_dst) {
        current = new_xbt_graph_node(graph, route->gw_dst->name(), nodes);
        current_name = route->gw_dst->name();
      } else {
        current = new_xbt_graph_node(graph, my_dst->name(), nodes);
        current_name = my_dst->name();
      }
      new_xbt_graph_edge(graph, previous, current, edges);
      XBT_DEBUG ("  %s -> %s", previous_name, current_name);

      xbt_dynar_free (&(route->link_list));
      xbt_free (route);
    }
  }
}


/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */
/* change a route containing link names into a route containing link entities */
sg_platf_route_cbarg_t AsRoutedGraph::newExtendedRoute(e_surf_routing_hierarchy_t hierarchy,
      sg_platf_route_cbarg_t routearg, int change_order) {

  sg_platf_route_cbarg_t result;
  char *link_name;
  unsigned int cpt;

  result = xbt_new0(s_sg_platf_route_cbarg_t, 1);
  result->link_list = xbt_dynar_new(sizeof(Link*), NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE
      || hierarchy == SURF_ROUTING_RECURSIVE,
      "The hierarchy of this AS is neither BASIC nor RECURSIVE, I'm lost here.");

  if (hierarchy == SURF_ROUTING_RECURSIVE) {

    xbt_assert(routearg->gw_src && routearg->gw_dst, "NULL is obviously a deficient gateway");

    /* remember not erase the gateway names */
    result->gw_src = routearg->gw_src;
    result->gw_dst = routearg->gw_dst;
  }

  xbt_dynar_foreach(routearg->link_list, cpt, link_name) {

    Link *link = Link::byName(link_name);
    if (link) {
      if (change_order)
        xbt_dynar_push(result->link_list, &link);
      else
        xbt_dynar_unshift(result->link_list, &link);
    } else
      THROWF(mismatch_error, 0, "Link '%s' not found", link_name);
  }

  return result;
}

void AsRoutedGraph::getRouteCheckParams(NetCard *src, NetCard *dst)
{
  xbt_assert(src,"Cannot find a route from NULL to %s", dst->name());
  xbt_assert(dst,"Cannot find a route from %s to NULL", src->name());

  As *src_as = src->containingAS();
  As *dst_as = dst->containingAS();

  xbt_assert(src_as == dst_as, "Internal error: %s@%s and %s@%s are not in the same AS as expected. Please report that bug.",
        src->name(), src_as->name_, dst->name(), dst_as->name_);

  xbt_assert(this == dst_as,
      "Internal error: route destination %s@%s is not in AS %s as expected (route source: %s@%s). Please report that bug.",
        src->name(), dst->name(),  src_as->name_, dst_as->name_,  name_);
}
void AsRoutedGraph::addRouteCheckParams(sg_platf_route_cbarg_t route) {
  const char *srcName = route->src;
  const char *dstName = route->dst;
  NetCard *src = sg_netcard_by_name_or_null(srcName);
  NetCard *dst = sg_netcard_by_name_or_null(dstName);

  if(!route->gw_dst && !route->gw_src) {
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", srcName, dstName);
    xbt_assert(src, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, srcName);
    xbt_assert(dst, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, dstName);
    xbt_assert(!xbt_dynar_is_empty(route->link_list), "Empty route (between %s and %s) forbidden.", srcName, dstName);
    xbt_assert(src->getRcType()==SURF_NETWORK_ELEMENT_HOST || src->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining a route, src must be an host or a router but '%s' is not. Did you meant to have an ASroute?", srcName);
    xbt_assert(dst->getRcType()==SURF_NETWORK_ELEMENT_HOST || dst->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining a route, dst must be an host or a router but '%s' is not. Did you meant to have an ASroute?", dstName);
  } else {
    XBT_DEBUG("Load ASroute from %s@%s to %s@%s", srcName, route->gw_src->name(), dstName, route->gw_dst->name());
    xbt_assert(src->getRcType()==SURF_NETWORK_ELEMENT_AS,
        "When defining an ASroute, src must be an AS but '%s' is not", srcName);
    xbt_assert(dst->getRcType()==SURF_NETWORK_ELEMENT_AS,
        "When defining an ASroute, dst must be an AS but '%s' is not", dstName);

    xbt_assert(route->gw_src->getRcType()==SURF_NETWORK_ELEMENT_HOST || route->gw_src->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining an ASroute, gw_src must be an host or a router but '%s' is not.", srcName);
    xbt_assert(route->gw_dst->getRcType()==SURF_NETWORK_ELEMENT_HOST || route->gw_dst->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining an ASroute, gw_dst must be an host or a router but '%s' is not.", dstName);

    xbt_assert(route->gw_src != route->gw_dst, "Cannot define an ASroute from '%s' to itself", route->gw_src->name());

    xbt_assert(src, "Cannot add a route from %s@%s to %s@%s: %s does not exist.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name(), srcName);
    xbt_assert(dst, "Cannot add a route from %s@%s to %s@%s: %s does not exist.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name(), dstName);
    xbt_assert(!xbt_dynar_is_empty(route->link_list), "Empty route (between %s@%s and %s@%s) forbidden.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name());
  }
}

}
}
