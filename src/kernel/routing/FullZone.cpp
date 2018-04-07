/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

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
FullZone::FullZone(NetZone* father, std::string name) : RoutedZone(father, name)
{
}

void FullZone::seal()
{
  unsigned int table_size = getTableSize();

  /* Create table if needed */
  if (not routing_table_)
    routing_table_ = new RouteCreationArgs*[table_size * table_size]();

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      RouteCreationArgs* e_route = TO_ROUTE_FULL(i, i);
      if (not e_route) {
        e_route = new RouteCreationArgs();
        e_route->link_list.push_back(surf_network_model->loopback_);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

FullZone::~FullZone()
{
  if (routing_table_) {
    unsigned int table_size = getTableSize();
    /* Delete routing table */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++)
        delete TO_ROUTE_FULL(i, j);
    delete[] routing_table_;
  }
}

void FullZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%u] to %s[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  unsigned int table_size        = getTableSize();
  RouteCreationArgs* e_route     = TO_ROUTE_FULL(src->id(), dst->id());

  if (e_route != nullptr) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    for (auto const& link : e_route->link_list) {
      res->link_list.push_back(link);
      if (lat)
        *lat += link->latency();
    }
  }
}

void FullZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                         std::vector<resource::LinkImpl*>& link_list, bool symmetrical)
{
  addRouteCheckParams(src, dst, gw_src, gw_dst, link_list, symmetrical);

  unsigned int table_size = getTableSize();

  if (not routing_table_)
    routing_table_ = new RouteCreationArgs*[table_size * table_size]();

  /* Check that the route does not already exist */
  if (gw_dst) // inter-zone route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->get_cname(), gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
  else
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->get_cname(),
               dst->get_cname());

  /* Add the route to the base */
  TO_ROUTE_FULL(src->id(), dst->id()) =
      newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, true);

  if (symmetrical == true && src != dst) {
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

    TO_ROUTE_FULL(dst->id(), src->id()) =
        newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, false);
  }
}
}
}
} // namespace
