/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/DijkstraZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/string.hpp"

#include <cfloat>
#include <queue>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_dijkstra, surf, "Routing part of surf -- dijkstra routing logic");

namespace simgrid {
namespace kernel {
namespace routing {

class GraphNodeData {
public:
  explicit GraphNodeData(int id) : id_(id) {}
  int id_;
  int graph_id_ = -1; /* used for caching internal graph id's */
};

DijkstraZone::DijkstraZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel, bool cached)
    : RoutedZone(father, name, netmodel), cached_(cached)
{
}

void DijkstraZone::route_graph_delete(xbt_graph_t g)
{
  xbt_graph_free_graph(g, [](void* n) { delete static_cast<simgrid::kernel::routing::GraphNodeData*>(n); },
                       [](void* e) { delete static_cast<simgrid::kernel::routing::RouteCreationArgs*>(e); }, nullptr);
}

void DijkstraZone::seal()
{
  unsigned int cursor;
  xbt_node_t node = nullptr;

  /* Add the loopback if needed */
  if (network_model_->loopback_ && hierarchy_ == RoutingMode::base) {
    xbt_dynar_foreach (xbt_graph_get_nodes(route_graph_.get()), cursor, node) {
      bool found = false;
      xbt_edge_t edge = nullptr;
      unsigned int cursor2;
      xbt_dynar_foreach (xbt_graph_node_get_outedges(node), cursor2, edge) {
        if (xbt_graph_edge_get_target(edge) == node) { // There is an edge from node to itself
          found = true;
          break;
        }
      }

      if (not found) {
        RouteCreationArgs* route = new simgrid::kernel::routing::RouteCreationArgs();
        route->link_list.push_back(network_model_->loopback_);
        xbt_graph_new_edge(route_graph_.get(), node, node, route);
      }
    }
  }

  /* initialize graph indexes in nodes after graph has been built */
  const_xbt_dynar_t nodes = xbt_graph_get_nodes(route_graph_.get());

  xbt_dynar_foreach (nodes, cursor, node) {
    GraphNodeData* data = static_cast<GraphNodeData*>(xbt_graph_node_get_data(node));
    data->graph_id_     = cursor;
  }
}

xbt_node_t DijkstraZone::route_graph_new_node(int id)
{
  xbt_node_t node = xbt_graph_new_node(route_graph_.get(), new GraphNodeData(id));
  graph_node_map_.emplace(id, node);

  return node;
}

xbt_node_t DijkstraZone::node_map_search(int id)
{
  auto ret = graph_node_map_.find(id);
  return ret == graph_node_map_.end() ? nullptr : ret->second;
}

/* Parsing */

void DijkstraZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  get_route_check_params(src, dst);
  int src_id = src->id();
  int dst_id = dst->id();

  const_xbt_dynar_t nodes = xbt_graph_get_nodes(route_graph_.get());

  /* Use the graph_node id mapping set to quickly find the nodes */
  const s_xbt_node_t* src_elm = node_map_search(src_id);
  const s_xbt_node_t* dst_elm = node_map_search(dst_id);

  int src_node_id = static_cast<GraphNodeData*>(xbt_graph_node_get_data(src_elm))->graph_id_;
  int dst_node_id = static_cast<GraphNodeData*>(xbt_graph_node_get_data(dst_elm))->graph_id_;

  /* if the src and dst are the same */
  if (src_node_id == dst_node_id) {
    const s_xbt_node_t* node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    const s_xbt_node_t* node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    const s_xbt_edge_t* edge     = xbt_graph_get_edge(route_graph_.get(), node_s_v, node_e_v);

    if (edge == nullptr)
      throw std::invalid_argument(xbt::string_printf("No route from '%s' to '%s'", src->get_cname(), dst->get_cname()));

    const RouteCreationArgs* e_route = static_cast<RouteCreationArgs*>(xbt_graph_edge_get_data(edge));

    for (auto const& link : e_route->link_list) {
      route->link_list.insert(route->link_list.begin(), link);
      if (lat)
        *lat += link->get_latency();
    }
  }

  auto elm                   = route_cache_.emplace(src_id, std::vector<int>());
  std::vector<int>& pred_arr = elm.first->second;

  if (elm.second) { /* new element was inserted (not cached mode, or cache miss) */
    int nr_nodes      = xbt_dynar_length(nodes);
    std::vector<double> cost_arr(nr_nodes); /* link cost from src to other hosts */
    pred_arr.resize(nr_nodes);              /* predecessors in path from src */
    typedef std::pair<double, int> Qelt;
    std::priority_queue<Qelt, std::vector<Qelt>, std::greater<Qelt>> pqueue;

    /* initialize */
    cost_arr[src_node_id] = 0.0;

    for (int i = 0; i < nr_nodes; i++) {
      if (i != src_node_id) {
        cost_arr[i] = DBL_MAX;
      }

      pred_arr[i] = 0;

      /* initialize priority queue */
      pqueue.emplace(cost_arr[i], i);
    }

    /* apply dijkstra using the indexes from the graph's node array */
    while (not pqueue.empty()) {
      int v_id = pqueue.top().second;
      pqueue.pop();
      const s_xbt_node_t* v_node = xbt_dynar_get_as(nodes, v_id, xbt_node_t);
      xbt_edge_t edge   = nullptr;
      unsigned int cursor;

      xbt_dynar_foreach (xbt_graph_node_get_outedges(v_node), cursor, edge) {
        const s_xbt_node_t* u_node           = xbt_graph_edge_get_target(edge);
        const GraphNodeData* data            = static_cast<GraphNodeData*>(xbt_graph_node_get_data(u_node));
        int u_id                           = data->graph_id_;
        const RouteCreationArgs* tmp_e_route = static_cast<RouteCreationArgs*>(xbt_graph_edge_get_data(edge));
        int cost_v_u                       = tmp_e_route->link_list.size(); /* count of links, old model assume 1 */

        if (cost_v_u + cost_arr[v_id] < cost_arr[u_id]) {
          pred_arr[u_id] = v_id;
          cost_arr[u_id] = cost_v_u + cost_arr[v_id];
          pqueue.emplace(cost_arr[u_id], u_id);
        }
      }
    }
  }

  /* compose route path with links */
  NetPoint* gw_src   = nullptr;
  NetPoint* first_gw = nullptr;

  for (int v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    const s_xbt_node_t* node_pred_v = xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    const s_xbt_node_t* node_v      = xbt_dynar_get_as(nodes, v, xbt_node_t);
    const s_xbt_edge_t* edge        = xbt_graph_get_edge(route_graph_.get(), node_pred_v, node_v);

    if (edge == nullptr)
      throw std::invalid_argument(xbt::string_printf("No route from '%s' to '%s'", src->get_cname(), dst->get_cname()));

    const RouteCreationArgs* e_route = static_cast<RouteCreationArgs*>(xbt_graph_edge_get_data(edge));

    const NetPoint* prev_gw_src = gw_src;
    gw_src                = e_route->gw_src;
    NetPoint* gw_dst      = e_route->gw_dst;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (hierarchy_ == RoutingMode::recursive && v != dst_node_id && gw_dst->get_name() != prev_gw_src->get_name()) {
      std::vector<resource::LinkImpl*> e_route_as_to_as;

      NetPoint* gw_dst_net_elm      = nullptr;
      NetPoint* prev_gw_src_net_elm = nullptr;
      get_global_route(gw_dst_net_elm, prev_gw_src_net_elm, e_route_as_to_as, nullptr);
      auto pos = route->link_list.begin();
      for (auto const& link : e_route_as_to_as) {
        route->link_list.insert(pos, link);
        if (lat)
          *lat += link->get_latency();
        pos++;
      }
    }

    for (auto const& link : e_route->link_list) {
      route->link_list.insert(route->link_list.begin(), link);
      if (lat)
        *lat += link->get_latency();
    }
  }

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = gw_src;
    route->gw_dst = first_gw;
  }

  if (not cached_)
    route_cache_.clear();
}

void DijkstraZone::add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                             std::vector<resource::LinkImpl*>& link_list, bool symmetrical)
{
  add_route_check_params(src, dst, gw_src, gw_dst, link_list, symmetrical);

  new_edge(src->id(), dst->id(), new_extended_route(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, 1));

  if (symmetrical == true)
    new_edge(dst->id(), src->id(), new_extended_route(hierarchy_, dst, src, gw_dst, gw_src, link_list, symmetrical, 0));
}

void DijkstraZone::new_edge(int src_id, int dst_id, RouteCreationArgs* route)
{
  XBT_DEBUG("Create Route from '%d' to '%d'", src_id, dst_id);

  // Get the extremities, or create them if they don't exist yet
  xbt_node_t src = node_map_search(src_id);
  if (src == nullptr)
    src = route_graph_new_node(src_id);

  xbt_node_t dst = node_map_search(dst_id);
  if (dst == nullptr)
    dst = route_graph_new_node(dst_id);

  // Make sure that this graph edge was not already added to the graph
  if (xbt_graph_get_edge(route_graph_.get(), src, dst) != nullptr) {
    if (route->gw_dst == nullptr || route->gw_src == nullptr)
      throw std::invalid_argument(
          xbt::string_printf("Route from %s to %s already exists", route->src->get_cname(), route->dst->get_cname()));
    else
      throw std::invalid_argument(xbt::string_printf("Route from %s@%s to %s@%s already exists",
                                                     route->src->get_cname(), route->gw_src->get_cname(),
                                                     route->dst->get_cname(), route->gw_dst->get_cname()));
  }

  // Finally add it
  xbt_graph_new_edge(route_graph_.get(), src, dst, route);
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
