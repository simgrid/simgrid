/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/AsFull.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i,j) routingTable_[(i)+(j)*table_size]

namespace simgrid {
namespace surf {
  AsFull::AsFull(const char*name)
    : AsRoutedGraph(name)
  {
  }

void AsFull::Seal() {
  int i;
  sg_platf_route_cbarg_t e_route;

  /* set utils vars */
  int table_size = (int)xbt_dynar_length(vertices_);

  /* Create table if necessary */
  if (!routingTable_)
    routingTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Add the loopback if needed */
  if (routing_platf->loopback_ && hierarchy_ == RoutingMode::base) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = new std::vector<Link*>();
        e_route->link_list->push_back(routing_platf->loopback_);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

AsFull::~AsFull(){
  if (routingTable_) {
    int table_size = (int)xbt_dynar_length(vertices_);
    int i, j;
    /* Delete routing table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++) {
        if (TO_ROUTE_FULL(i,j)){
          delete TO_ROUTE_FULL(i,j)->link_list;
          xbt_free(TO_ROUTE_FULL(i,j));
        }
      }
    xbt_free(routingTable_);
  }
}

void AsFull::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t res, double *lat)
{
  XBT_DEBUG("full_get_route_and_latency from %s[%d] to %s[%d]",
      src->name(), src->id(), dst->name(), dst->id());

  /* set utils vars */
  size_t table_size = xbt_dynar_length(vertices_);

  sg_platf_route_cbarg_t e_route = NULL;

  e_route = TO_ROUTE_FULL(src->id(), dst->id());

  if (e_route) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    for (auto link : *e_route->link_list) {
      res->link_list->push_back(link);
      if (lat)
        *lat += static_cast<Link*>(link)->getLatency();
    }
  }
}

void AsFull::addRoute(sg_platf_route_cbarg_t route)
{
  const char *src = route->src;
  const char *dst = route->dst;
  NetCard *src_net_elm = sg_netcard_by_name_or_null(src);
  NetCard *dst_net_elm = sg_netcard_by_name_or_null(dst);

  addRouteCheckParams(route);

  size_t table_size = xbt_dynar_length(vertices_);

  if (!routingTable_)
    routingTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Check that the route does not already exist */
  if (route->gw_dst) // AS route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id()),
        "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
        src,route->gw_src->name(),dst,route->gw_dst->name());
  else
    xbt_assert(nullptr == TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id()),
        "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src,dst);

  /* Add the route to the base */
  TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id()) = newExtendedRoute(hierarchy_, route, 1);
  TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id())->link_list->shrink_to_fit();

  if (route->symmetrical == true && src_net_elm != dst_net_elm) {
    if (route->gw_dst && route->gw_src) {
      NetCard* gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }
    if (route->gw_dst) // AS route (to adapt the error message, if any)
      xbt_assert(nullptr == TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst,route->gw_dst->name(),src,route->gw_src->name());
    else
      xbt_assert(nullptr == TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id()),
          "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.", dst,src);

    TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id()) = newExtendedRoute(hierarchy_, route, 0);
    TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id())->link_list->shrink_to_fit();
  }
}

}
}
