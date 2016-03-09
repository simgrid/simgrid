/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/dynar.h"
#include "src/surf/AsFloyd.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

#define TO_FLOYD_COST(i,j) (costTable_)[(i)+(j)*table_size]
#define TO_FLOYD_PRED(i,j) (predecessorTable_)[(i)+(j)*table_size]
#define TO_FLOYD_LINK(i,j) (linkTable_)[(i)+(j)*table_size]

namespace simgrid {
namespace surf {

AsFloyd::AsFloyd(const char*name)
  : AsRoutedGraph(name)
{
  predecessorTable_ = NULL;
  costTable_ = NULL;
  linkTable_ = NULL;
}

AsFloyd::~AsFloyd(){
  int i, j;
  int table_size = (int)xbt_dynar_length(vertices_);
  if (linkTable_ == NULL) // Dealing with a parse error in the file?
    return;
  /* Delete link_table */
  for (i = 0; i < table_size; i++)
    for (j = 0; j < table_size; j++)
      routing_route_free(TO_FLOYD_LINK(i, j));
  xbt_free(linkTable_);

  xbt_free(predecessorTable_);
  xbt_free(costTable_);
}

void AsFloyd::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t route, double *lat)
{
  size_t table_size = xbt_dynar_length(vertices_);

  getRouteCheckParams(src, dst);

  /* create a result route */
  xbt_dynar_t route_stack = xbt_dynar_new(sizeof(sg_platf_route_cbarg_t), NULL);
  int pred;
  int cur = dst->id();
  do {
    pred = TO_FLOYD_PRED(src->id(), cur);
    if (pred == -1)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->name(), dst->name());
    xbt_dynar_push_as(route_stack, sg_platf_route_cbarg_t, TO_FLOYD_LINK(pred, cur));
    cur = pred;
  } while (cur != src->id());

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = xbt_dynar_getlast_as(route_stack, sg_platf_route_cbarg_t)->gw_src;
    route->gw_dst = xbt_dynar_getfirst_as(route_stack, sg_platf_route_cbarg_t)->gw_dst;
  }

  sg_netcard_t prev_dst_gw = NULL;
  while (!xbt_dynar_is_empty(route_stack)) {
    sg_platf_route_cbarg_t e_route = xbt_dynar_pop_as(route_stack, sg_platf_route_cbarg_t);

    if (hierarchy_ == RoutingMode::recursive && prev_dst_gw != NULL && strcmp(prev_dst_gw->name(), e_route->gw_src->name())) {
      routing_platf->getRouteAndLatency(prev_dst_gw, e_route->gw_src, route->link_list, lat);
    }

    for (auto link: *e_route->link_list) {
      route->link_list->push_back(link);
      if (lat)
        *lat += link->getLatency();
    }

    prev_dst_gw = e_route->gw_dst;
  }
  xbt_dynar_free(&route_stack);
}

void AsFloyd::addRoute(sg_platf_route_cbarg_t route)
{
  /* set the size of table routing */
  int table_size = (int)xbt_dynar_length(vertices_);

  NetCard *src = sg_netcard_by_name_or_null(route->src);
  NetCard *dst = sg_netcard_by_name_or_null(route->dst);

  addRouteCheckParams(route);

  if(!linkTable_) {
    /* Create Cost, Predecessor and Link tables */
    costTable_ = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
    predecessorTable_ = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
    linkTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);    /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (int i = 0; i < table_size; i++)
      for (int j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = NULL;
      }
  }

  /* Check that the route does not already exist */
  if (route->gw_dst) // AS route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_FLOYD_LINK(src->id(), dst->id()),
        "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
        src->name(),route->gw_src->name(),dst->name(),route->gw_dst->name());
  else
    xbt_assert(nullptr == TO_FLOYD_LINK(src->id(), dst->id()),
        "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->name(),dst->name());

  TO_FLOYD_LINK(src->id(), dst->id()) = newExtendedRoute(hierarchy_, route, 1);
  TO_FLOYD_PRED(src->id(), dst->id()) = src->id();
  TO_FLOYD_COST(src->id(), dst->id()) = (TO_FLOYD_LINK(src->id(), dst->id()))->link_list->size();


  if (route->symmetrical == true) {
    if (route->gw_dst) // AS route (to adapt the error message, if any)
      xbt_assert(nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->name(),route->gw_dst->name(),src->name(),route->gw_src->name());
    else
      xbt_assert(nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
          "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
          dst->name(),src->name());

    if(route->gw_dst && route->gw_src) {
      NetCard* gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }

    if(!route->gw_src && !route->gw_dst)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst->name(), src->name());
    else
      XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst->name(),
          route->gw_src->name(), src->name(), route->gw_dst->name());

    TO_FLOYD_LINK(dst->id(), src->id()) = newExtendedRoute(hierarchy_, route, 0);
    TO_FLOYD_PRED(dst->id(), src->id()) = dst->id();
    TO_FLOYD_COST(dst->id(), src->id()) = (TO_FLOYD_LINK(dst->id(), src->id()))->link_list->size();   /* count of links, old model assume 1 */
  }
}

void AsFloyd::Seal(){

  /* set the size of table routing */
  size_t table_size = xbt_dynar_length(vertices_);

  if(!linkTable_) {
    /* Create Cost, Predecessor and Link tables */
    costTable_ = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
    predecessorTable_ = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
    linkTable_ = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);    /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = NULL;
      }
  }

  /* Add the loopback if needed */
  if (routing_platf->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      sg_platf_route_cbarg_t e_route = TO_FLOYD_LINK(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = new std::vector<Link*>();
        e_route->link_list->push_back(routing_platf->loopback_);
        TO_FLOYD_LINK(i, i) = e_route;
        TO_FLOYD_PRED(i, i) = i;
        TO_FLOYD_COST(i, i) = 1;
      }
    }
  }
  /* Calculate path costs */
  for (unsigned int c = 0; c < table_size; c++) {
    for (unsigned int a = 0; a < table_size; a++) {
      for (unsigned int b = 0; b < table_size; b++) {
        if (TO_FLOYD_COST(a, c) < DBL_MAX && TO_FLOYD_COST(c, b) < DBL_MAX) {
          if (TO_FLOYD_COST(a, b) == DBL_MAX ||
              (TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b) <
                  TO_FLOYD_COST(a, b))) {
            TO_FLOYD_COST(a, b) =
                TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b);
            TO_FLOYD_PRED(a, b) = TO_FLOYD_PRED(c, b);
          }
        }
      }
    }
  }
}

}
}
