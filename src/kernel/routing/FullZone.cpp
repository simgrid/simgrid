/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/FullZone.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i, j) routingTable_[(i) + (j)*table_size]

namespace simgrid {
namespace kernel {
namespace routing {
AsFull::AsFull(As* father, const char* name) : AsRoutedGraph(father, name)
{
}

void AsFull::seal()
{
  int i;
  sg_platf_route_cbarg_t e_route;

  /* set utils vars */
  int table_size = static_cast<int>(vertices_.size());

  /* Create table if necessary */
  if (!routingTable_)
    routingTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route            = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src    = nullptr;
        e_route->gw_dst    = nullptr;
        e_route->link_list = new std::vector<Link*>();
        e_route->link_list->push_back(surf_network_model->loopback_);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

AsFull::~AsFull()
{
  if (routingTable_) {
    int table_size = static_cast<int>(vertices_.size());
    /* Delete routing table */
    for (int i = 0; i < table_size; i++)
      for (int j = 0; j < table_size; j++) {
        if (TO_ROUTE_FULL(i, j)) {
          delete TO_ROUTE_FULL(i, j)->link_list;
          xbt_free(TO_ROUTE_FULL(i, j));
        }
      }
    xbt_free(routingTable_);
  }
}

void AsFull::getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%d] to %s[%d]", src->cname(), src->id(), dst->cname(), dst->id());

  size_t table_size              = vertices_.size();
  sg_platf_route_cbarg_t e_route = TO_ROUTE_FULL(src->id(), dst->id());

  if (e_route != nullptr) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    for (auto link : *e_route->link_list) {
      res->link_list->push_back(link);
      if (lat)
        *lat += link->latency();
    }
  }
}

void AsFull::addRoute(sg_platf_route_cbarg_t route)
{
  NetCard* src = route->src;
  NetCard* dst = route->dst;
  addRouteCheckParams(route);

  size_t table_size = vertices_.size();

  if (!routingTable_)
    routingTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Check that the route does not already exist */
  if (route->gw_dst) // AS route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->cname(), route->gw_src->cname(), dst->cname(), route->gw_dst->cname());
  else
    xbt_assert(nullptr == TO_ROUTE_FULL(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->cname(),
               dst->cname());

  /* Add the route to the base */
  TO_ROUTE_FULL(src->id(), dst->id()) = newExtendedRoute(hierarchy_, route, 1);
  TO_ROUTE_FULL(src->id(), dst->id())->link_list->shrink_to_fit();

  if (route->symmetrical == true && src != dst) {
    if (route->gw_dst && route->gw_src) {
      NetCard* gw_tmp = route->gw_src;
      route->gw_src   = route->gw_dst;
      route->gw_dst   = gw_tmp;
    }
    if (route->gw_dst) // AS route (to adapt the error message, if any)
      xbt_assert(
          nullptr == TO_ROUTE_FULL(dst->id(), src->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->cname(), route->gw_dst->cname(), src->cname(), route->gw_src->cname());
    else
      xbt_assert(nullptr == TO_ROUTE_FULL(dst->id(), src->id()),
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->cname(), src->cname());

    TO_ROUTE_FULL(dst->id(), src->id()) = newExtendedRoute(hierarchy_, route, 0);
    TO_ROUTE_FULL(dst->id(), src->id())->link_list->shrink_to_fit();
  }
}
}
}
} // namespace
