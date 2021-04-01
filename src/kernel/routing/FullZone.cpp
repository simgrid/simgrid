/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i, j) routing_table_[(i) + (j)*table_size]

namespace simgrid {
namespace kernel {
namespace routing {

void FullZone::do_seal()
{
  unsigned int table_size = get_table_size();

  /* Create table if needed */
  if (routing_table_.empty())
    routing_table_.resize(table_size * table_size, nullptr);

  /* Add the loopback if needed */
  if (get_network_model()->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      RouteCreationArgs* route = TO_ROUTE_FULL(i, i);
      if (not route) {
        route = new RouteCreationArgs();
        route->link_list.push_back(get_network_model()->loopback_);
        TO_ROUTE_FULL(i, i) = route;
      }
    }
  }
}

FullZone::~FullZone()
{
  /* Delete routing table */
  for (auto const* route : routing_table_)
    delete route;
}

void FullZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%u] to %s[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  unsigned int table_size          = get_table_size();
  const RouteCreationArgs* e_route = TO_ROUTE_FULL(src->id(), dst->id());

  if (e_route != nullptr) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    for (auto const& link : e_route->link_list) {
      res->link_list.push_back(link);
      if (lat)
        *lat += link->get_latency();
    }
  }
}

void FullZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         std::vector<resource::LinkImpl*>& link_list, bool symmetrical)
{
  add_route_check_params(src, dst, gw_src, gw_dst, link_list, symmetrical);

  unsigned int table_size = get_table_size();

  if (routing_table_.empty())
    routing_table_.resize(table_size * table_size, nullptr);

  /* Check that the route does not already exist */
  if (gw_dst && gw_src) // inter-zone route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->get_cname(), gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
  else
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->get_cname(),
               dst->get_cname());

  /* Add the route to the base */
  TO_ROUTE_FULL(src->id(), dst->id()) = new_extended_route(hierarchy_, gw_src, gw_dst, link_list, true);

  if (symmetrical && src != dst) {
    if (gw_dst && gw_src) {
      NetPoint* gw_tmp = gw_src;
      gw_src           = gw_dst;
      gw_dst           = gw_tmp;
    }
    if (gw_dst && gw_src) // inter-zone route (to adapt the error message, if any)
      xbt_assert(
          nullptr == TO_ROUTE_FULL(dst->id(), src->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->get_cname(), gw_dst->get_cname(), src->get_cname(), gw_src->get_cname());
    else
      xbt_assert(nullptr == TO_ROUTE_FULL(dst->id(), src->id()),
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->get_cname(), src->get_cname());

    TO_ROUTE_FULL(dst->id(), src->id()) = new_extended_route(hierarchy_, gw_src, gw_dst, link_list, false);
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
