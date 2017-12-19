/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/FloydZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/log.h"

#include <cfloat>
#include <limits>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

#define TO_FLOYD_COST(i, j) (costTable_)[(i) + (j)*table_size]
#define TO_FLOYD_PRED(i, j) (predecessorTable_)[(i) + (j)*table_size]
#define TO_FLOYD_LINK(i, j) (linkTable_)[(i) + (j)*table_size]

namespace simgrid {
namespace kernel {
namespace routing {

FloydZone::FloydZone(NetZone* father, std::string name) : RoutedZone(father, name)
{
  predecessorTable_ = nullptr;
  costTable_        = nullptr;
  linkTable_        = nullptr;
}

FloydZone::~FloydZone()
{
  if (linkTable_ == nullptr) // Dealing with a parse error in the file?
    return;
  unsigned int table_size = getTableSize();
  /* Delete link_table */
  for (unsigned int i = 0; i < table_size; i++)
    for (unsigned int j = 0; j < table_size; j++)
      delete TO_FLOYD_LINK(i, j);
  delete[] linkTable_;

  delete[] predecessorTable_;
  delete[] costTable_;
}

void FloydZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* lat)
{
  unsigned int table_size = getTableSize();

  getRouteCheckParams(src, dst);

  /* create a result route */
  std::vector<sg_platf_route_cbarg_t> route_stack;
  unsigned int cur = dst->id();
  do {
    int pred = TO_FLOYD_PRED(src->id(), cur);
    if (pred == -1)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->getCname(), dst->getCname());
    route_stack.push_back(TO_FLOYD_LINK(pred, cur));
    cur = pred;
  } while (cur != src->id());

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = route_stack.back()->gw_src;
    route->gw_dst = route_stack.front()->gw_dst;
  }

  sg_netpoint_t prev_dst_gw = nullptr;
  while (not route_stack.empty()) {
    sg_platf_route_cbarg_t e_route = route_stack.back();
    route_stack.pop_back();
    if (hierarchy_ == RoutingMode::recursive && prev_dst_gw != nullptr &&
        prev_dst_gw->getCname() != e_route->gw_src->getCname()) {
      getGlobalRoute(prev_dst_gw, e_route->gw_src, route->link_list, lat);
    }

    for (auto const& link : e_route->link_list) {
      route->link_list.push_back(link);
      if (lat)
        *lat += link->latency();
    }

    prev_dst_gw = e_route->gw_dst;
  }
}

void FloydZone::addRoute(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                         kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                         std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical)
{
  /* set the size of table routing */
  unsigned int table_size = getTableSize();

  addRouteCheckParams(src, dst, gw_src, gw_dst, link_list, symmetrical);

  if (not linkTable_) {
    /* Create Cost, Predecessor and Link tables */
    costTable_        = new double[table_size * table_size];                 /* link cost from host to host */
    predecessorTable_ = new int[table_size * table_size];                    /* predecessor host numbers */
    linkTable_        = new sg_platf_route_cbarg_t[table_size * table_size]; /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = nullptr;
      }
  }

  /* Check that the route does not already exist */
  if (gw_dst) // netzone route (to adapt the error message, if any)
    xbt_assert(nullptr == TO_FLOYD_LINK(src->id(), dst->id()),
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->getCname(), gw_src->getCname(), dst->getCname(), gw_dst->getCname());
  else
    xbt_assert(nullptr == TO_FLOYD_LINK(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->getCname(),
               dst->getCname());

  TO_FLOYD_LINK(src->id(), dst->id()) =
      newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, 1);
  TO_FLOYD_PRED(src->id(), dst->id()) = src->id();
  TO_FLOYD_COST(src->id(), dst->id()) = (TO_FLOYD_LINK(src->id(), dst->id()))->link_list.size();

  if (symmetrical == true) {
    if (gw_dst) // netzone route (to adapt the error message, if any)
      xbt_assert(
          nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->getCname(), gw_dst->getCname(), src->getCname(), gw_src->getCname());
    else
      xbt_assert(nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->getCname(), src->getCname());

    if (gw_dst && gw_src) {
      NetPoint* gw_tmp = gw_src;
      gw_src           = gw_dst;
      gw_dst           = gw_tmp;
    }

    if (not gw_src || not gw_dst)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst->getCname(), src->getCname());
    else
      XBT_DEBUG("Load NetzoneRoute from \"%s(%s)\" to \"%s(%s)\"", dst->getCname(), gw_src->getCname(), src->getCname(),
                gw_dst->getCname());

    TO_FLOYD_LINK(dst->id(), src->id()) =
        newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, 0);
    TO_FLOYD_PRED(dst->id(), src->id()) = dst->id();
    TO_FLOYD_COST(dst->id(), src->id()) =
        (TO_FLOYD_LINK(dst->id(), src->id()))->link_list.size(); /* count of links, old model assume 1 */
  }
}

void FloydZone::seal()
{
  /* set the size of table routing */
  unsigned int table_size = getTableSize();

  if (not linkTable_) {
    /* Create Cost, Predecessor and Link tables */
    costTable_        = new double[table_size * table_size];                 /* link cost from host to host */
    predecessorTable_ = new int[table_size * table_size];                    /* predecessor host numbers */
    linkTable_        = new sg_platf_route_cbarg_t[table_size * table_size]; /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = nullptr;
      }
  }

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      sg_platf_route_cbarg_t e_route = TO_FLOYD_LINK(i, i);
      if (not e_route) {
        e_route            = new s_sg_platf_route_cbarg_t;
        e_route->gw_src    = nullptr;
        e_route->gw_dst    = nullptr;
        e_route->link_list.push_back(surf_network_model->loopback_);
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
        if (TO_FLOYD_COST(a, c) < DBL_MAX && TO_FLOYD_COST(c, b) < DBL_MAX &&
            (fabs(TO_FLOYD_COST(a, b) - DBL_MAX) < std::numeric_limits<double>::epsilon() ||
             (TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b) < TO_FLOYD_COST(a, b)))) {
          TO_FLOYD_COST(a, b) = TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b);
          TO_FLOYD_PRED(a, b) = TO_FLOYD_PRED(c, b);
        }
      }
    }
  }
}
}
}
}
