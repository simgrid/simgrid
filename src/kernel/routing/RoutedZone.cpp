/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "xbt/graph.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");

/* ***************************************************************** */
/* *********************** GENERIC METHODS ************************* */

xbt_node_t new_xbt_graph_node(const s_xbt_graph_t* graph, const char* name, std::map<std::string, xbt_node_t>* nodes)
{
  auto elm = nodes->find(name);
  if (elm == nodes->end()) {
    xbt_node_t ret = xbt_graph_new_node(graph, xbt_strdup(name));
    nodes->insert({name, ret});
    return ret;
  } else
    return elm->second;
}

xbt_edge_t new_xbt_graph_edge(const s_xbt_graph_t* graph, xbt_node_t s, xbt_node_t d,
                              std::map<std::string, xbt_edge_t>* edges)
{
  const auto* sn   = static_cast<const char*>(xbt_graph_node_get_data(s));
  const auto* dn   = static_cast<const char*>(xbt_graph_node_get_data(d));
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

RoutedZone::RoutedZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : NetZoneImpl(father, name, netmodel)
{
}

void RoutedZone::get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t>* nodes,
                           std::map<std::string, xbt_edge_t>* edges)
{
  std::vector<NetPoint*> vertices = get_vertices();

  for (auto const& my_src : vertices) {
    for (auto const& my_dst : vertices) {
      if (my_src == my_dst)
        continue;

      RouteCreationArgs route;

      get_local_route(my_src, my_dst, &route, nullptr);

      XBT_DEBUG("get_route_and_latency %s -> %s", my_src->get_cname(), my_dst->get_cname());

      xbt_node_t current;
      xbt_node_t previous;
      const char *previous_name;
      const char *current_name;

      if (route.gw_src) {
        previous      = new_xbt_graph_node(graph, route.gw_src->get_cname(), nodes);
        previous_name = route.gw_src->get_cname();
      } else {
        previous      = new_xbt_graph_node(graph, my_src->get_cname(), nodes);
        previous_name = my_src->get_cname();
      }

      for (auto const& link : route.link_list) {
        const char* link_name = link->get_cname();
        current               = new_xbt_graph_node(graph, link_name, nodes);
        current_name          = link_name;
        new_xbt_graph_edge(graph, previous, current, edges);
        XBT_DEBUG("  %s -> %s", previous_name, current_name);
        previous      = current;
        previous_name = current_name;
      }

      if (route.gw_dst) {
        current      = new_xbt_graph_node(graph, route.gw_dst->get_cname(), nodes);
        current_name = route.gw_dst->get_cname();
      } else {
        current      = new_xbt_graph_node(graph, my_dst->get_cname(), nodes);
        current_name = my_dst->get_cname();
      }
      new_xbt_graph_edge(graph, previous, current, edges);
      XBT_DEBUG("  %s -> %s", previous_name, current_name);
    }
  }
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */
/* change a route containing link names into a route containing link entities */
RouteCreationArgs* RoutedZone::new_extended_route(RoutingMode hierarchy, NetPoint* gw_src, NetPoint* gw_dst,
                                                  const std::vector<resource::LinkImpl*>& link_list,
                                                  bool preserve_order)
{
  auto* result = new RouteCreationArgs();

  xbt_assert(hierarchy == RoutingMode::base || hierarchy == RoutingMode::recursive,
             "The hierarchy of this netzone is neither BASIC nor RECURSIVE, I'm lost here.");

  if (hierarchy == RoutingMode::recursive) {
    xbt_assert(gw_src && gw_dst, "nullptr is obviously a deficient gateway");

    result->gw_src = gw_src;
    result->gw_dst = gw_dst;
  }

  if (preserve_order)
    result->link_list = link_list;
  else
    result->link_list.assign(link_list.rbegin(), link_list.rend()); // reversed
  result->link_list.shrink_to_fit();

  return result;
}

void RoutedZone::get_route_check_params(NetPoint* src, NetPoint* dst) const
{
  xbt_assert(src, "Cannot find a route from nullptr to %s", dst->get_cname());
  xbt_assert(dst, "Cannot find a route from %s to nullptr", src->get_cname());

  const NetZoneImpl* src_as = src->get_englobing_zone();
  const NetZoneImpl* dst_as = dst->get_englobing_zone();

  xbt_assert(src_as == dst_as,
             "Internal error: %s@%s and %s@%s are not in the same netzone as expected. Please report that bug.",
             src->get_cname(), src_as->get_cname(), dst->get_cname(), dst_as->get_cname());

  xbt_assert(this == dst_as,
             "Internal error: route destination %s@%s is not in netzone %s as expected (route source: "
             "%s@%s). Please report that bug.",
             src->get_cname(), dst->get_cname(), src_as->get_cname(), dst_as->get_cname(), get_cname());
}
void RoutedZone::add_route_check_params(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                        const std::vector<resource::LinkImpl*>& link_list, bool symmetrical) const
{
  const char* srcName = src->get_cname();
  const char* dstName = dst->get_cname();

  if (not gw_dst || not gw_src) {
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", srcName, dstName);
    xbt_assert(src, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, srcName);
    xbt_assert(dst, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, dstName);
    xbt_assert(not link_list.empty(), "Empty route (between %s and %s) forbidden.", srcName, dstName);
    xbt_assert(not src->is_netzone(),
               "When defining a route, src cannot be a netzone such as '%s'. Did you meant to have a NetzoneRoute?",
               srcName);
    xbt_assert(not dst->is_netzone(),
               "When defining a route, dst cannot be a netzone such as '%s'. Did you meant to have a NetzoneRoute?",
               dstName);
  } else {
    XBT_DEBUG("Load NetzoneRoute from %s@%s to %s@%s", srcName, gw_src->get_cname(), dstName, gw_dst->get_cname());
    xbt_assert(src->is_netzone(), "When defining a NetzoneRoute, src must be a netzone but '%s' is not", srcName);
    xbt_assert(dst->is_netzone(), "When defining a NetzoneRoute, dst must be a netzone but '%s' is not", dstName);

    xbt_assert(gw_src->is_host() || gw_src->is_router(),
               "When defining a NetzoneRoute, gw_src must be a host or a router but '%s' is not.", srcName);
    xbt_assert(gw_dst->is_host() || gw_dst->is_router(),
               "When defining a NetzoneRoute, gw_dst must be a host or a router but '%s' is not.", dstName);

    xbt_assert(gw_src != gw_dst, "Cannot define a NetzoneRoute from '%s' to itself", gw_src->get_cname());

    xbt_assert(src, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->get_cname(), dstName,
               gw_dst->get_cname(), srcName);
    xbt_assert(dst, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->get_cname(), dstName,
               gw_dst->get_cname(), dstName);
    xbt_assert(not link_list.empty(), "Empty route (between %s@%s and %s@%s) forbidden.", srcName, gw_src->get_cname(),
               dstName, gw_dst->get_cname());
  }

  s4u::NetZone::on_route_creation(symmetrical, src, dst, gw_src, gw_dst, link_list);
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
