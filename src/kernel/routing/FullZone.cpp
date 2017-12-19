/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/FullZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i, j) routingTable_[(i) + (j)*table_size]

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
  if (not routingTable_)
    routingTable_ = new sg_platf_route_cbarg_t[table_size * table_size]();

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      sg_platf_route_cbarg_t e_route = TO_ROUTE_FULL(i, i);
      if (not e_route) {
        e_route            = new s_sg_platf_route_cbarg_t;
        e_route->gw_src    = nullptr;
        e_route->gw_dst    = nullptr;
        e_route->link_list.push_back(surf_network_model->loopback_);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

FullZone::~FullZone()
{
  if (routingTable_) {
    unsigned int table_size = getTableSize();
    /* Delete routing table */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++)
        delete TO_ROUTE_FULL(i, j);
    delete[] routingTable_;
  }
}

void FullZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%u] to %s[%u]", src->getCname(), src->id(), dst->getCname(), dst->id());

  unsigned int table_size        = getTableSize();
  sg_platf_route_cbarg_t e_route = TO_ROUTE_FULL(src->id(), dst->id());

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

void FullZone::addRoute(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical)
{
  addRouteCheckParams(src, dst, gw_src, gw_dst, link_list, symmetrical);

  unsigned int table_size = getTableSize();

  if (not routingTable_)
    routingTable_ = new sg_platf_route_cbarg_t[table_size * table_size]();

  /* Check that the route does not already exist */
  if (gw_dst) // inter-zone route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->getCname(), gw_src->getCname(), dst->getCname(), gw_dst->getCname());
  else
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->getCname(),
               dst->getCname());

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
          dst->getCname(), gw_dst->getCname(), src->getCname(), gw_src->getCname());
    else
      xbt_assert(nullptr == TO_ROUTE_FULL(dst->id(), src->id()),
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->getCname(), src->getCname());

    TO_ROUTE_FULL(dst->id(), src->id()) =
        newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, false);
  }
}
}
}
} // namespace
