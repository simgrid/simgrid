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
                                  double* latency, std::unordered_set<resource::LinkImpl*>& added_links)
{
  for (auto* link : links) {
    /* do not add duplicated links */
    if (added_links.find(link) != added_links.end())
      continue;
    added_links.insert(link);
    if (latency)
      *latency += link->get_latency();
    route->link_list.push_back(link);
  }
}

void StarZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* latency)
{
  XBT_VERB("StarZone getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(),
           dst->id());

  xbt_assert(((src == dst) and (loopback_.find(src->id()) != loopback_.end())) or
                 (links_up_.find(src->id()) != links_up_.end()),
             "StarZone routing (%s - %s): no link UP from source node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());
  xbt_assert(((src == dst) and (loopback_.find(dst->id()) != loopback_.end())) or
                 (links_down_.find(dst->id()) != links_down_.end()),
             "StarZone routing (%s - %s): no link DOWN to destination node. Did you use add_route() to set it?",
             src->get_cname(), dst->get_cname());

  std::unordered_set<resource::LinkImpl*> added_links;
  /* loopback */
  if ((src == dst) and (loopback_.find(src->id()) != loopback_.end())) {
    add_links_to_route(loopback_[src->id()], route, latency, added_links);
    return;
  }

  /* going UP */
  add_links_to_route(links_up_[src->id()], route, latency, added_links);

  /* going DOWN */
  add_links_to_route(links_down_[dst->id()], route, latency, added_links);
}

void StarZone::get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                         std::map<std::string, xbt_edge_t, std::less<>>* edges)
{
}

void StarZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         const std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical)
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

  /* loopback */
  if (src == dst) {
    loopback_[src->id()] = link_list;
  } else {
    /* src to everyone */
    if (src) {
      links_up_[src->id()] = link_list;
      if (symmetrical) {
        links_down_[src->id()] = link_list;
        /* reverse it for down/symmetrical links */
        std::reverse(links_down_[src->id()].begin(), links_down_[src->id()].end());
      }
    }
    /* dst to everyone */
    if (dst) {
      links_down_[dst->id()] = link_list;
    }
  }
  s4u::NetZone::on_route_creation(symmetrical, gw_src, gw_dst, gw_src, gw_dst, link_list);
}

void StarZone::do_seal()
{
  /* add default empty links if nothing was configured by user */
  for (auto const& node : get_vertices()) {
    if ((links_up_.find(node->id()) == links_up_.end()) and (links_down_.find(node->id()) == links_down_.end())) {
      links_down_[node->id()] = {};
      links_up_[node->id()]   = {};
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
