/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/FloydZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp"
#include "xbt/string.hpp"

#include <climits>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

void FloydZone::init_tables(unsigned int table_size)
{
  if (link_table_.size() != table_size) {
    /* Resize and initialize Cost, Predecessor and Link tables */
    cost_table_.resize(table_size);
    link_table_.resize(table_size);
    predecessor_table_.resize(table_size);
    for (auto& cost : cost_table_)
      cost.resize(table_size, ULONG_MAX); /* link cost from host to host */
    for (auto& link : link_table_)
      link.resize(table_size); /* actual link between src and dst */
    for (auto& predecessor : predecessor_table_)
      predecessor.resize(table_size, -1); /* predecessor host numbers */
  }
}

void FloydZone::get_local_route(const NetPoint* src, const NetPoint* dst, Route* route, double* lat)
{
  get_route_check_params(src, dst);

  /* create a result route */
  std::vector<Route*> route_stack;
  unsigned long cur = dst->id();
  do {
    int pred = predecessor_table_[src->id()][cur];
    if (pred == -1)
      throw std::invalid_argument(xbt::string_printf("No route from '%s' to '%s'", src->get_cname(), dst->get_cname()));
    route_stack.push_back(link_table_[pred][cur].get());
    cur = pred;
  } while (cur != src->id());

  if (get_hierarchy() == RoutingMode::recursive) {
    route->gw_src_ = route_stack.back()->gw_src_;
    route->gw_dst_ = route_stack.front()->gw_dst_;
  }

  const NetPoint* prev_dst_gw = nullptr;
  while (not route_stack.empty()) {
    const Route* e_route = route_stack.back();
    route_stack.pop_back();
    if (get_hierarchy() == RoutingMode::recursive && prev_dst_gw != nullptr &&
        prev_dst_gw->get_cname() != e_route->gw_src_->get_cname()) {
      get_global_route(prev_dst_gw, e_route->gw_src_, route->link_list_, lat);
    }

    add_link_latency(route->link_list_, e_route->link_list_, lat);

    prev_dst_gw = e_route->gw_dst_;
  }
}

void FloydZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                          const std::vector<s4u::LinkInRoute>& link_list, bool symmetrical)
{
  /* set the size of table routing */
  unsigned int table_size = get_table_size();
  init_tables(table_size);

  add_route_check_params(src, dst, gw_src, gw_dst, link_list, symmetrical);

  /* Check that the route does not already exist */
  if (gw_dst && gw_src) // netzone route (to adapt the error message, if any)
    xbt_assert(nullptr == link_table_[src->id()][dst->id()],
               "The route between %s@%s and %s@%s already exists (Rq: routes are symmetrical by default).",
               src->get_cname(), gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
  else
    xbt_assert(nullptr == link_table_[src->id()][dst->id()],
               "The route between %s and %s already exists (Rq: routes are symmetrical by default).", src->get_cname(),
               dst->get_cname());

  link_table_[src->id()][dst->id()] = std::unique_ptr<Route>(
      new_extended_route(get_hierarchy(), gw_src, gw_dst, get_link_list_impl(link_list, false), true));
  predecessor_table_[src->id()][dst->id()] = src->id();
  cost_table_[src->id()][dst->id()]        = link_table_[src->id()][dst->id()]->link_list_.size();

  if (symmetrical) {
    if (gw_dst && gw_src) // netzone route (to adapt the error message, if any)
      xbt_assert(
          nullptr == link_table_[dst->id()][src->id()],
          "The route between %s@%s and %s@%s already exists. You should not declare the reverse path as symmetrical.",
          dst->get_cname(), gw_dst->get_cname(), src->get_cname(), gw_src->get_cname());
    else
      xbt_assert(nullptr == link_table_[dst->id()][src->id()],
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

    link_table_[dst->id()][src->id()] = std::unique_ptr<Route>(
        new_extended_route(get_hierarchy(), gw_src, gw_dst, get_link_list_impl(link_list, true), false));
    predecessor_table_[dst->id()][src->id()] = dst->id();
    cost_table_[dst->id()][src->id()] =
        link_table_[dst->id()][src->id()]->link_list_.size(); /* count of links, old model assume 1 */
  }
}

void FloydZone::do_seal()
{
  /* set the size of table routing */
  unsigned int table_size = get_table_size();
  init_tables(table_size);

  /* Add the loopback if needed */
  if (get_network_model()->loopback_ && get_hierarchy() == RoutingMode::base) {
    for (unsigned int i = 0; i < table_size; i++) {
      auto& route = link_table_[i][i];
      if (not route) {
        route.reset(new Route());
        route->link_list_.push_back(get_network_model()->loopback_);
        predecessor_table_[i][i] = i;
        cost_table_[i][i]        = 1;
      }
    }
  }
  /* Calculate path costs */
  for (unsigned int c = 0; c < table_size; c++) {
    for (unsigned int a = 0; a < table_size; a++) {
      for (unsigned int b = 0; b < table_size; b++) {
        if (cost_table_[a][c] < ULONG_MAX && cost_table_[c][b] < ULONG_MAX &&
            (cost_table_[a][b] == ULONG_MAX || (cost_table_[a][c] + cost_table_[c][b] < cost_table_[a][b]))) {
          cost_table_[a][b]        = cost_table_[a][c] + cost_table_[c][b];
          predecessor_table_[a][b] = predecessor_table_[c][b];
        }
      }
    }
  }
}
} // namespace routing
} // namespace kernel

namespace s4u {
NetZone* create_floyd_zone(const std::string& name)
{
  return (new kernel::routing::FloydZone(name))->get_iface();
}
} // namespace s4u

} // namespace simgrid
