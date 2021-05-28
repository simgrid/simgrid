/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/StarZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/string.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_star, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {
StarZone::StarZone(const std::string& name) : ClusterZone(name) {}

void StarZone::add_links_to_route(const std::vector<resource::LinkImpl*>& links, Route* route, double* latency,
                                  std::unordered_set<resource::LinkImpl*>& added_links) const
{
  for (auto* link : links) {
    /* do not add duplicated links in route->link_list_ */
    if (not added_links.insert(link).second)
      continue;
    if (latency)
      *latency += link->get_latency();
    route->link_list_.push_back(link);
  }
}

void StarZone::get_local_route(NetPoint* src, NetPoint* dst, Route* route, double* latency)
{
  XBT_VERB("StarZone getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(),
           dst->id());

  const auto& src_route = routes_.at(src->id());
  const auto& dst_route = routes_.at(dst->id());
  std::unordered_set<resource::LinkImpl*> added_links;
  /* loopback */
  if (src == dst && src_route.has_loopback()) {
    add_links_to_route(src_route.loopback, route, latency, added_links);
    return;
  }

  xbt_assert(src_route.has_links_up(),
             "StarZone routing (%s - %s): no link UP from source node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());
  xbt_assert(dst_route.has_links_down(),
             "StarZone routing (%s - %s): no link DOWN to destination node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());

  /* going UP */
  add_links_to_route(src_route.links_up, route, latency, added_links);

  /* going DOWN */
  add_links_to_route(dst_route.links_down, route, latency, added_links);
  /* gateways */
  route->gw_src_ = src_route.gateway;
  route->gw_dst_ = dst_route.gateway;
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
      new_xbt_graph_edge(graph, current, previous, edges);
      previous = current;
    }
    new_xbt_graph_edge(graph, src_node, previous, edges);
  }
}

void StarZone::check_add_route_param(const NetPoint* src, const NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                     bool symmetrical) const
{
  const char* src_name = src ? src->get_cname() : "nullptr";
  const char* dst_name = dst ? dst->get_cname() : "nullptr";

  if ((not src && not dst) || (dst && src && src != dst))
    throw std::invalid_argument(xbt::string_printf(
        "Cannot add route from %s to %s. In a StarZone, route must be:  i) from source netpoint to everyone, ii) from "
        "everyone to a single netpoint or iii) loopback, same source and destination",
        src_name, dst_name));

  if (symmetrical && not src)
    throw std::invalid_argument(xbt::string_printf("Cannot add route from %s to %s. In a StarZone, symmetrical routes "
                                                   "must be set from source to everyone (not the contrary)",
                                                   src_name, dst_name));

  if (src && src->is_netzone()) {
    if (not gw_src)
      throw std::invalid_argument(xbt::string_printf(
          "StarZone::add_route(): source %s is a netzone but gw_src isn't configured", src->get_cname()));
    if (gw_src->is_netzone())
      throw std::invalid_argument(
          xbt::string_printf("StarZone::add_route(): src(%s) is a netzone, gw_src(%s) cannot be a netzone",
                             src->get_cname(), gw_src->get_cname()));

    const auto* netzone_src = get_netzone_recursive(src);
    if (not netzone_src->is_component_recursive(gw_src))
      throw std::invalid_argument(xbt::string_printf(
          "Invalid NetzoneRoute from %s@%s to %s: gw_src %s belongs to %s, not to %s.", src_name, gw_src->get_cname(),
          dst_name, gw_src->get_cname(), gw_src->get_englobing_zone()->get_cname(), src_name));
  }

  if (dst && dst->is_netzone()) {
    if (not gw_dst)
      throw std::invalid_argument(xbt::string_printf(
          "StarZone::add_route(): destination %s is a netzone but gw_dst isn't configured", dst->get_cname()));
    if (gw_dst->is_netzone())
      throw std::invalid_argument(
          xbt::string_printf("StarZone::add_route(): dst(%s) is a netzone, gw_dst(%s) cannot be a netzone",
                             dst->get_cname(), gw_dst->get_cname()));

    const auto* netzone_dst = get_netzone_recursive(dst);
    if (not netzone_dst->is_component_recursive(gw_dst))
      throw std::invalid_argument(xbt::string_printf(
          "Invalid NetzoneRoute from %s@%s to %s: gw_dst %s belongs to %s, not to %s.", dst_name, gw_dst->get_cname(),
          src_name, gw_dst->get_cname(), gw_dst->get_englobing_zone()->get_cname(), dst_name));
  }
}

void StarZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         const std::vector<kernel::resource::LinkImpl*>& link_list_, bool symmetrical)
{
  check_add_route_param(src, dst, gw_src, gw_dst, symmetrical);

  s4u::NetZone::on_route_creation(symmetrical, src, dst, gw_src, gw_dst, link_list_);

  /* loopback */
  if (src == dst) {
    routes_[src->id()].loopback = link_list_;
  } else {
    /* src to everyone */
    if (src) {
      auto& route        = routes_[src->id()];
      route.links_up     = link_list_;
      route.gateway      = gw_src;
      route.links_up_set = true;
      if (symmetrical) {
        /* reverse it for down/symmetrical links */
        route.links_down.assign(link_list_.rbegin(), link_list_.rend());
        route.links_down_set = true;
      }
    }
    /* dst to everyone */
    if (dst) {
      auto& route          = routes_[dst->id()];
      route.links_down     = link_list_;
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
