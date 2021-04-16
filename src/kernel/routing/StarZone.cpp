/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/StarZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp" // RouteCreationArgs and friends

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_star, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {
StarZone::StarZone(const std::string& name) : NetZoneImpl(name) {}

void StarZone::add_links_to_route(const std::vector<resource::LinkImpl*>& links, RouteCreationArgs* route,
                                  double* latency, std::unordered_set<resource::LinkImpl*>& added_links) const
{
  for (auto* link : links) {
    /* do not add duplicated links in route->link_list */
    if (not added_links.insert(link).second)
      continue;
    if (latency)
      *latency += link->get_latency();
    route->link_list.push_back(link);
  }
}

void StarZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* latency)
{
  XBT_VERB("StarZone getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(),
           dst->id());

  bool has_loopback = ((src == dst) and (routes_[src->id()].has_loopback()));
  xbt_assert((has_loopback) or (routes_[src->id()].has_links_up()),
             "StarZone routing (%s - %s): no link UP from source node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());
  xbt_assert((has_loopback) or (routes_[dst->id()].has_links_down()),
             "StarZone routing (%s - %s): no link DOWN to destination node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());

  std::unordered_set<resource::LinkImpl*> added_links;
  /* loopback */
  if ((src == dst) and (routes_[src->id()].has_loopback())) {
    add_links_to_route(routes_[src->id()].loopback, route, latency, added_links);
    return;
  }

  /* going UP */
  add_links_to_route(routes_[src->id()].links_up, route, latency, added_links);

  /* going DOWN */
  add_links_to_route(routes_[dst->id()].links_down, route, latency, added_links);
  /* gateways */
  route->gw_src = routes_[src->id()].gateway;
  route->gw_dst = routes_[dst->id()].gateway;
}

void StarZone::get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                         std::map<std::string, xbt_edge_t, std::less<>>* edges)
{
  xbt_node_t star_node = new_xbt_graph_node(graph, get_cname(), nodes);

  for (auto const& src : get_vertices()) {
    /* going up */
    xbt_node_t src_node = new_xbt_graph_node(graph, src->get_cname(), nodes);
    xbt_node_t previous = src_node;
    for (auto const* link : routes_[src->id()].links_up) {
      xbt_node_t current = new_xbt_graph_node(graph, link->get_cname(), nodes);
      new_xbt_graph_edge(graph, previous, current, edges);
      previous = current;
    }
    new_xbt_graph_edge(graph, previous, star_node, edges);
    /* going down */
    previous = star_node;
    for (auto const* link : routes_[src->id()].links_down) {
      xbt_node_t current = new_xbt_graph_node(graph, link->get_cname(), nodes);
      new_xbt_graph_edge(graph, previous, current, edges);
      previous = current;
    }
    new_xbt_graph_edge(graph, previous, src_node, edges);
  }
}

void StarZone::check_add_route_param(const NetPoint* src, const NetPoint* dst, const NetPoint* gw_src,
                                     const NetPoint* gw_dst, bool symmetrical) const
{
  const char* src_name = src ? src->get_cname() : "nullptr";
  const char* dst_name = dst ? dst->get_cname() : "nullptr";

  xbt_assert((src == dst) or (not src and dst) or (src and not dst),
             "Cannot add route from %s to %s. In a StarZone, route must be:  i) from source host to everyone, ii) from "
             "everyone to a single host or iii) loopback, same source and destination",
             src_name, dst_name);
  xbt_assert((not symmetrical) or (symmetrical and src),
             "Cannot add route from %s to %s. In a StarZone, symmetrical routes must be set from source to everyone "
             "(not the contrary).",
             src_name, dst_name);

  if (src and src->is_netzone()) {
    xbt_assert(gw_src, "add_route(): source %s is a netzone but gw_src isn't configured", src->get_cname());
    xbt_assert(not gw_src->is_netzone(), "add_route(): src(%s) is a netzone, gw_src(%s) cannot be a netzone",
               src->get_cname(), gw_src->get_cname());
  }

  if (dst and dst->is_netzone()) {
    xbt_assert(gw_dst, "add_route(): destination %s is a netzone but gw_dst isn't configured", dst->get_cname());
    xbt_assert(not gw_dst->is_netzone(), "add_route(): dst(%s) is a netzone, gw_dst(%s) cannot be a netzone",
               dst->get_cname(), gw_dst->get_cname());
  }
}

void StarZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         const std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical)
{
  check_add_route_param(src, dst, gw_src, gw_dst, symmetrical);

  s4u::NetZone::on_route_creation(symmetrical, gw_src, gw_dst, gw_src, gw_dst, link_list);

  /* loopback */
  if (src == dst) {
    routes_[src->id()].loopback = link_list;
  } else {
    /* src to everyone */
    if (src) {
      auto& route        = routes_[src->id()];
      route.links_up     = link_list;
      route.gateway      = gw_src;
      route.links_up_set = true;
      if (symmetrical) {
        /* reverse it for down/symmetrical links */
        route.links_down.assign(link_list.rbegin(), link_list.rend());
        route.links_down_set = true;
      }
    }
    /* dst to everyone */
    if (dst) {
      auto& route          = routes_[dst->id()];
      route.links_down     = link_list;
      route.gateway        = gw_dst;
      route.links_down_set = true;
    }
  }
}

void StarZone::do_seal()
{
  /* add default empty links if nothing was configured by user */
  for (auto const& node : get_vertices()) {
    auto route = routes_.emplace(node->id(), StarRoute());
    if (route.second) {
      route.first->second.links_down_set = true;
      route.first->second.links_up_set   = true;
    }
  }
}

} // namespace routing
} // namespace kernel

namespace s4u {
NetZone* create_star_zone(const std::string& name)
{
  return (new kernel::routing::StarZone(name))->get_iface();
}
} // namespace s4u

} // namespace simgrid
