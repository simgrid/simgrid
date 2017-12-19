/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "xbt/graph.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include "src/kernel/routing/NetPoint.hpp"
#include "src/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");

/* ***************************************************************** */
/* *********************** GENERIC METHODS ************************* */

static const char* instr_node_name(xbt_node_t node)
{
  void* data = xbt_graph_node_get_data(node);
  return static_cast<const char*>(data);
}

xbt_node_t new_xbt_graph_node(xbt_graph_t graph, const char* name, std::map<std::string, xbt_node_t>* nodes)
{

  auto elm = nodes->find(name);
  if (elm == nodes->end()) {
    xbt_node_t ret = xbt_graph_new_node(graph, xbt_strdup(name));
    nodes->insert({name, ret});
    return ret;
  } else
    return elm->second;
}

xbt_edge_t new_xbt_graph_edge(xbt_graph_t graph, xbt_node_t s, xbt_node_t d, std::map<std::string, xbt_edge_t>* edges)
{
  const char* sn = instr_node_name(s);
  const char* dn = instr_node_name(d);
  std::string name = std::string(sn) + dn;

  auto elm = edges->find(name);
  if (elm == edges->end()) {
    name = std::string(dn) + sn;
    elm  = edges->find(name);
  }

  if (elm == edges->end()) {
    xbt_edge_t ret = xbt_graph_new_edge(graph, s, d, nullptr);
    edges->insert({name, ret});
    return ret;
  } else
    return elm->second;
}

namespace simgrid {
namespace kernel {
namespace routing {

RoutedZone::RoutedZone(NetZone* father, std::string name) : NetZoneImpl(father, name)
{
}

void RoutedZone::getGraph(xbt_graph_t graph, std::map<std::string, xbt_node_t>* nodes,
                          std::map<std::string, xbt_edge_t>* edges)
{
  std::vector<kernel::routing::NetPoint*> vertices = getVertices();

  for (auto const& my_src : vertices) {
    for (auto const& my_dst : vertices) {
      if (my_src == my_dst)
        continue;

      sg_platf_route_cbarg_t route = new s_sg_platf_route_cbarg_t;

      getLocalRoute(my_src, my_dst, route, nullptr);

      XBT_DEBUG("get_route_and_latency %s -> %s", my_src->getCname(), my_dst->getCname());

      xbt_node_t current;
      xbt_node_t previous;
      const char *previous_name;
      const char *current_name;

      if (route->gw_src) {
        previous      = new_xbt_graph_node(graph, route->gw_src->getCname(), nodes);
        previous_name = route->gw_src->getCname();
      } else {
        previous      = new_xbt_graph_node(graph, my_src->getCname(), nodes);
        previous_name = my_src->getCname();
      }

      for (auto const& link : route->link_list) {
        const char* link_name = link->getCname();
        current               = new_xbt_graph_node(graph, link_name, nodes);
        current_name          = link_name;
        new_xbt_graph_edge(graph, previous, current, edges);
        XBT_DEBUG("  %s -> %s", previous_name, current_name);
        previous      = current;
        previous_name = current_name;
      }

      if (route->gw_dst) {
        current      = new_xbt_graph_node(graph, route->gw_dst->getCname(), nodes);
        current_name = route->gw_dst->getCname();
      } else {
        current      = new_xbt_graph_node(graph, my_dst->getCname(), nodes);
        current_name = my_dst->getCname();
      }
      new_xbt_graph_edge(graph, previous, current, edges);
      XBT_DEBUG("  %s -> %s", previous_name, current_name);

      delete route;
    }
  }
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */
/* change a route containing link names into a route containing link entities */
sg_platf_route_cbarg_t RoutedZone::newExtendedRoute(RoutingMode hierarchy, NetPoint* src, NetPoint* dst,
                                                    NetPoint* gw_src, NetPoint* gw_dst,
                                                    std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical,
                                                    bool change_order)
{
  sg_platf_route_cbarg_t result = new s_sg_platf_route_cbarg_t;

  xbt_assert(hierarchy == RoutingMode::base || hierarchy == RoutingMode::recursive,
             "The hierarchy of this netzone is neither BASIC nor RECURSIVE, I'm lost here.");

  if (hierarchy == RoutingMode::recursive) {
    xbt_assert(gw_src && gw_dst, "nullptr is obviously a deficient gateway");

    result->gw_src = gw_src;
    result->gw_dst = gw_dst;
  }

  for (auto const& link : link_list) {
    if (change_order)
      result->link_list.push_back(link);
    else
      result->link_list.insert(result->link_list.begin(), link);
  }
  result->link_list.shrink_to_fit();

  return result;
}

void RoutedZone::getRouteCheckParams(NetPoint* src, NetPoint* dst)
{
  xbt_assert(src, "Cannot find a route from nullptr to %s", dst->getCname());
  xbt_assert(dst, "Cannot find a route from %s to nullptr", src->getCname());

  NetZone* src_as = src->netzone();
  NetZone* dst_as = dst->netzone();

  xbt_assert(src_as == dst_as,
             "Internal error: %s@%s and %s@%s are not in the same netzone as expected. Please report that bug.",
             src->getCname(), src_as->getCname(), dst->getCname(), dst_as->getCname());

  xbt_assert(this == dst_as, "Internal error: route destination %s@%s is not in netzone %s as expected (route source: "
                             "%s@%s). Please report that bug.",
             src->getCname(), dst->getCname(), src_as->getCname(), dst_as->getCname(), getCname());
}
void RoutedZone::addRouteCheckParams(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                                     kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                                     std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical)
{
  const char* srcName = src->getCname();
  const char* dstName = dst->getCname();

  if (not gw_dst || not gw_src) {
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", srcName, dstName);
    xbt_assert(src, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, srcName);
    xbt_assert(dst, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, dstName);
    xbt_assert(not link_list.empty(), "Empty route (between %s and %s) forbidden.", srcName, dstName);
    xbt_assert(not src->isNetZone(),
               "When defining a route, src cannot be a netzone such as '%s'. Did you meant to have an NetzoneRoute?",
               srcName);
    xbt_assert(not dst->isNetZone(),
               "When defining a route, dst cannot be a netzone such as '%s'. Did you meant to have an NetzoneRoute?",
               dstName);
  } else {
    XBT_DEBUG("Load NetzoneRoute from %s@%s to %s@%s", srcName, gw_src->getCname(), dstName, gw_dst->getCname());
    xbt_assert(src->isNetZone(), "When defining a NetzoneRoute, src must be a netzone but '%s' is not", srcName);
    xbt_assert(dst->isNetZone(), "When defining a NetzoneRoute, dst must be a netzone but '%s' is not", dstName);

    xbt_assert(gw_src->isHost() || gw_src->isRouter(),
               "When defining a NetzoneRoute, gw_src must be an host or a router but '%s' is not.", srcName);
    xbt_assert(gw_dst->isHost() || gw_dst->isRouter(),
               "When defining a NetzoneRoute, gw_dst must be an host or a router but '%s' is not.", dstName);

    xbt_assert(gw_src != gw_dst, "Cannot define an NetzoneRoute from '%s' to itself", gw_src->getCname());

    xbt_assert(src, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->getCname(), dstName,
               gw_dst->getCname(), srcName);
    xbt_assert(dst, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->getCname(), dstName,
               gw_dst->getCname(), dstName);
    xbt_assert(not link_list.empty(), "Empty route (between %s@%s and %s@%s) forbidden.", srcName, gw_src->getCname(),
               dstName, gw_dst->getCname());
  }

  onRouteCreation(symmetrical, src, dst, gw_src, gw_dst, link_list);
}
}
}
}
