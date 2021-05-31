/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

void FullZone::check_routing_table()
{
  unsigned int table_size = get_table_size();
  /* assure routing_table is table_size X table_size */
  if (routing_table_.size() != table_size) {
    routing_table_.resize(table_size);
    for (auto& j : routing_table_) {
      j.resize(table_size);
    }
  }
}

void FullZone::do_seal()
{
  check_routing_table();
  /* Add the loopback if needed */
  if (get_network_model()->loopback_ && get_hierarchy() == RoutingMode::base) {
    for (unsigned int i = 0; i < get_table_size(); i++) {
      auto& route = routing_table_[i][i];
      if (not route) {
        route.reset(new Route());
        route->link_list_.push_back(get_network_model()->loopback_);
      }
    }
  }
}

void FullZone::get_local_route(const NetPoint* src, const NetPoint* dst, Route* res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%u] to %s[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  const auto& e_route = routing_table_[src->id()][dst->id()];

  if (e_route != nullptr) {
    res->gw_src_ = e_route->gw_src_;
    res->gw_dst_ = e_route->gw_dst_;
    for (auto const& link : e_route->link_list_) {
      res->link_list_.push_back(link);
      if (lat)
        *lat += link->get_latency();
    }
  }
}

void FullZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         const std::vector<resource::LinkImpl*>& link_list, bool symmetrical)
{
  add_route_check_params(src, dst, gw_src, gw_dst, link_list, symmetrical);

  check_routing_table();

  /* Check that the route does not already exist */
  if (gw_dst && gw_src) // inter-zone route (to adapt the error message, if any)
    xbt_assert(nullptr == routing_table_[src->id()][dst->id()],
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->get_cname(), gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
  else
    xbt_assert(nullptr == routing_table_[src->id()][dst->id()],
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->get_cname(),
               dst->get_cname());

  /* Add the route to the base */
  routing_table_[src->id()][dst->id()] =
      std::unique_ptr<Route>(new_extended_route(get_hierarchy(), gw_src, gw_dst, link_list, true));

  if (symmetrical && src != dst) {
    if (gw_dst && gw_src) {
      NetPoint* gw_tmp = gw_src;
      gw_src           = gw_dst;
      gw_dst           = gw_tmp;
    }
    if (gw_dst && gw_src) // inter-zone route (to adapt the error message, if any)
      xbt_assert(
          nullptr == routing_table_[dst->id()][src->id()],
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->get_cname(), gw_dst->get_cname(), src->get_cname(), gw_src->get_cname());
    else
      xbt_assert(nullptr == routing_table_[dst->id()][src->id()],
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->get_cname(), src->get_cname());

    routing_table_[dst->id()][src->id()] =
        std::unique_ptr<Route>(new_extended_route(get_hierarchy(), gw_src, gw_dst, link_list, false));
  }
}
} // namespace routing
} // namespace kernel

namespace s4u {
NetZone* create_full_zone(const std::string& name)
{
  return (new kernel::routing::FullZone(name))->get_iface();
}
} // namespace s4u

} // namespace simgrid
