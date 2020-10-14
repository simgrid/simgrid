/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/FloydZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/string.hpp"

#include <cfloat>
#include <limits>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

#define TO_FLOYD_COST(i, j) (cost_table_)[(i) + (j)*table_size]
#define TO_FLOYD_PRED(i, j) (predecessor_table_)[(i) + (j)*table_size]
#define TO_FLOYD_LINK(i, j) (link_table_)[(i) + (j)*table_size]

namespace simgrid {
namespace kernel {
namespace routing {

FloydZone::FloydZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : RoutedZone(father, name, netmodel)
{
}

FloydZone::~FloydZone()
{
  if (link_table_ == nullptr) // Dealing with a parse error in the file?
    return;
  unsigned int table_size = get_table_size();
  /* Delete link_table */
  for (unsigned int i = 0; i < table_size; i++)
    for (unsigned int j = 0; j < table_size; j++)
      delete TO_FLOYD_LINK(i, j);
  delete[] link_table_;

  delete[] predecessor_table_;
  delete[] cost_table_;
}

void FloydZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  unsigned int table_size = get_table_size();

  get_route_check_params(src, dst);

  /* create a result route */
  std::vector<RouteCreationArgs*> route_stack;
  unsigned int cur = dst->id();
  do {
    int pred = TO_FLOYD_PRED(src->id(), cur);
    if (pred == -1)
      throw std::invalid_argument(xbt::string_printf("No route from '%s' to '%s'", src->get_cname(), dst->get_cname()));
    route_stack.push_back(TO_FLOYD_LINK(pred, cur));
    cur = pred;
  } while (cur != src->id());

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = route_stack.back()->gw_src;
    route->gw_dst = route_stack.front()->gw_dst;
  }

  NetPoint* prev_dst_gw = nullptr;
  while (not route_stack.empty()) {
    const RouteCreationArgs* e_route = route_stack.back();
    route_stack.pop_back();
    if (hierarchy_ == RoutingMode::recursive && prev_dst_gw != nullptr &&
        prev_dst_gw->get_cname() != e_route->gw_src->get_cname()) {
      get_global_route(prev_dst_gw, e_route->gw_src, route->link_list, lat);
    }

    for (auto const& link : e_route->link_list) {
      route->link_list.push_back(link);
      if (lat)
        *lat += link->get_latency();
    }

    prev_dst_gw = e_route->gw_dst;
  }
}

void FloydZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                          std::vector<resource::LinkImpl*>& link_list, bool symmetrical)
{
  /* set the size of table routing */
  unsigned int table_size = get_table_size();

  add_route_check_params(src, dst, gw_src, gw_dst, link_list, symmetrical);

  if (not link_table_) {
    /* Create Cost, Predecessor and Link tables */
    cost_table_        = new double[table_size * table_size];             /* link cost from host to host */
    predecessor_table_ = new int[table_size * table_size];                /* predecessor host numbers */
    link_table_        = new RouteCreationArgs*[table_size * table_size]; /* actual link between src and dst */

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
               src->get_cname(), gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
  else
    xbt_assert(nullptr == TO_FLOYD_LINK(src->id(), dst->id()),
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->get_cname(),
               dst->get_cname());

  TO_FLOYD_LINK(src->id(), dst->id()) = new_extended_route(hierarchy_, gw_src, gw_dst, link_list, true);
  TO_FLOYD_PRED(src->id(), dst->id()) = src->id();
  TO_FLOYD_COST(src->id(), dst->id()) = (TO_FLOYD_LINK(src->id(), dst->id()))->link_list.size();

  if (symmetrical == true) {
    if (gw_dst) // netzone route (to adapt the error message, if any)
      xbt_assert(
          nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->get_cname(), gw_dst->get_cname(), src->get_cname(), gw_src->get_cname());
    else
      xbt_assert(nullptr == TO_FLOYD_LINK(dst->id(), src->id()),
                 "The route between %s and %s already exists. You should not declare the reverse path as symmetrical.",
                 dst->get_cname(), src->get_cname());

    if (gw_dst && gw_src) {
      NetPoint* gw_tmp = gw_src;
      gw_src           = gw_dst;
      gw_dst           = gw_tmp;
    }

    if (not gw_src || not gw_dst)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst->get_cname(), src->get_cname());
    else
      XBT_DEBUG("Load NetzoneRoute from \"%s(%s)\" to \"%s(%s)\"", dst->get_cname(), gw_src->get_cname(),
                src->get_cname(), gw_dst->get_cname());

    TO_FLOYD_LINK(dst->id(), src->id()) = new_extended_route(hierarchy_, gw_src, gw_dst, link_list, false);
    TO_FLOYD_PRED(dst->id(), src->id()) = dst->id();
    TO_FLOYD_COST(dst->id(), src->id()) =
        (TO_FLOYD_LINK(dst->id(), src->id()))->link_list.size(); /* count of links, old model assume 1 */
  }
}

void FloydZone::seal()
{
  /* set the size of table routing */
  unsigned int table_size = get_table_size();

  if (not link_table_) {
    /* Create Cost, Predecessor and Link tables */
    cost_table_        = new double[table_size * table_size];             /* link cost from host to host */
    predecessor_table_ = new int[table_size * table_size];                /* predecessor host numbers */
    link_table_        = new RouteCreationArgs*[table_size * table_size]; /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (unsigned int i = 0; i < table_size; i++)
      for (unsigned int j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = nullptr;
      }
  }

  /* Add the loopback if needed */
  if (network_model_->loopback_ && hierarchy_ == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      RouteCreationArgs* route = TO_FLOYD_LINK(i, i);
      if (not route) {
        route = new RouteCreationArgs();
        route->link_list.push_back(network_model_->loopback_);
        TO_FLOYD_LINK(i, i) = route;
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
