/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/DijkstraZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"

#include <cfloat>
#include <queue>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_dijkstra, surf, "Routing part of surf -- dijkstra routing logic");

/* Free functions */

static void graph_node_data_free(void* n)
{
  graph_node_data_t data = static_cast<graph_node_data_t>(n);
  delete data;
}

static void graph_edge_data_free(void* e)
{
  sg_platf_route_cbarg_t e_route = static_cast<sg_platf_route_cbarg_t>(e);
  delete e_route;
}

/* Utility functions */

namespace simgrid {
namespace kernel {
namespace routing {
void DijkstraZone::seal()
{
  unsigned int cursor;
  xbt_node_t node = nullptr;

  /* Create the topology graph */
  if (not routeGraph_)
    routeGraph_ = xbt_graph_new_graph(1, nullptr);

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    xbt_dynar_foreach (xbt_graph_get_nodes(routeGraph_), cursor, node) {

      bool found = false;
      xbt_edge_t edge = nullptr;
      unsigned int cursor2;
      xbt_dynar_foreach (xbt_graph_node_get_outedges(node), cursor2, edge) {
        if (xbt_graph_edge_get_target(edge) == node) {
          found = true;
          break;
        }
      }

      if (not found) {
        sg_platf_route_cbarg_t e_route = new s_sg_platf_route_cbarg_t;
        e_route->link_list.push_back(surf_network_model->loopback_);
        xbt_graph_new_edge(routeGraph_, node, node, e_route);
      }
    }
  }

  /* initialize graph indexes in nodes after graph has been built */
  xbt_dynar_t nodes = xbt_graph_get_nodes(routeGraph_);

  xbt_dynar_foreach (nodes, cursor, node) {
    graph_node_data_t data = static_cast<graph_node_data_t>(xbt_graph_node_get_data(node));
    data->graph_id         = cursor;
  }
}

xbt_node_t DijkstraZone::routeGraphNewNode(int id, int graph_id)
{
  graph_node_data_t data = new s_graph_node_data_t;
  data->id       = id;
  data->graph_id = graph_id;

  xbt_node_t node                = xbt_graph_new_node(routeGraph_, data);
  graphNodeMap_.emplace(id, node);

  return node;
}

xbt_node_t DijkstraZone::nodeMapSearch(int id)
{
  auto ret = graphNodeMap_.find(id);
  return ret == graphNodeMap_.end() ? nullptr : ret->second;
}

/* Parsing */

void DijkstraZone::newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route)
{
  XBT_DEBUG("Load Route from \"%d\" to \"%d\"", src_id, dst_id);
  xbt_node_t src = nullptr;
  xbt_node_t dst = nullptr;

  xbt_node_t src_elm = nodeMapSearch(src_id);
  xbt_node_t dst_elm = nodeMapSearch(dst_id);

  if (src_elm)
    src = src_elm;

  if (dst_elm)
    dst = dst_elm;

  /* add nodes if they don't exist in the graph */
  if (src_id == dst_id && src == nullptr && dst == nullptr) {
    src = this->routeGraphNewNode(src_id, -1);
    dst = src;
  } else {
    if (src == nullptr) {
      src = this->routeGraphNewNode(src_id, -1);
    }
    if (dst == nullptr) {
      dst = this->routeGraphNewNode(dst_id, -1);
    }
  }

  /* add link as edge to graph */
  xbt_graph_new_edge(routeGraph_, src, dst, e_route);
}

void DijkstraZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* lat)
{
  getRouteCheckParams(src, dst);
  int src_id = src->id();
  int dst_id = dst->id();

  xbt_dynar_t nodes = xbt_graph_get_nodes(routeGraph_);

  /* Use the graph_node id mapping set to quickly find the nodes */
  xbt_node_t src_elm = nodeMapSearch(src_id);
  xbt_node_t dst_elm = nodeMapSearch(dst_id);

  int src_node_id = static_cast<graph_node_data_t>(xbt_graph_node_get_data(src_elm))->graph_id;
  int dst_node_id = static_cast<graph_node_data_t>(xbt_graph_node_get_data(dst_elm))->graph_id;

  /* if the src and dst are the same */
  if (src_node_id == dst_node_id) {

    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    xbt_edge_t edge     = xbt_graph_get_edge(routeGraph_, node_s_v, node_e_v);

    if (edge == nullptr)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->getCname(), dst->getCname());

    sg_platf_route_cbarg_t e_route = static_cast<sg_platf_route_cbarg_t>(xbt_graph_edge_get_data(edge));

    for (auto const& link : e_route->link_list) {
      route->link_list.insert(route->link_list.begin(), link);
      if (lat)
        *lat += static_cast<surf::LinkImpl*>(link)->latency();
    }
  }

  auto elm                   = routeCache_.emplace(src_id, std::vector<int>());
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
      xbt_node_t v_node = xbt_dynar_get_as(nodes, v_id, xbt_node_t);
      xbt_edge_t edge   = nullptr;
      unsigned int cursor;

      xbt_dynar_foreach (xbt_graph_node_get_outedges(v_node), cursor, edge) {
        xbt_node_t u_node                  = xbt_graph_edge_get_target(edge);
        graph_node_data_t data             = static_cast<graph_node_data_t>(xbt_graph_node_get_data(u_node));
        int u_id                           = data->graph_id;
        sg_platf_route_cbarg_t tmp_e_route = static_cast<sg_platf_route_cbarg_t>(xbt_graph_edge_get_data(edge));
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
  NetPoint* gw_src = nullptr;
  NetPoint* gw_dst;
  NetPoint* first_gw            = nullptr;

  for (int v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v = xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v      = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge        = xbt_graph_get_edge(routeGraph_, node_pred_v, node_v);

    if (edge == nullptr)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->getCname(), dst->getCname());

    sg_platf_route_cbarg_t e_route = static_cast<sg_platf_route_cbarg_t>(xbt_graph_edge_get_data(edge));

    NetPoint* prev_gw_src          = gw_src;
    gw_src                         = e_route->gw_src;
    gw_dst                         = e_route->gw_dst;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (hierarchy_ == RoutingMode::recursive && v != dst_node_id && gw_dst->getName() != prev_gw_src->getName()) {
      std::vector<surf::LinkImpl*> e_route_as_to_as;

      NetPoint* gw_dst_net_elm      = nullptr;
      NetPoint* prev_gw_src_net_elm = nullptr;
      getGlobalRoute(gw_dst_net_elm, prev_gw_src_net_elm, e_route_as_to_as, nullptr);
      auto pos = route->link_list.begin();
      for (auto const& link : e_route_as_to_as) {
        route->link_list.insert(pos, link);
        if (lat)
          *lat += link->latency();
        pos++;
      }
    }

    for (auto const& link : e_route->link_list) {
      route->link_list.insert(route->link_list.begin(), link);
      if (lat)
        *lat += static_cast<surf::LinkImpl*>(link)->latency();
    }
  }

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = gw_src;
    route->gw_dst = first_gw;
  }

  if (not cached_)
    routeCache_.clear();
}

DijkstraZone::~DijkstraZone()
{
  xbt_graph_free_graph(routeGraph_, &graph_node_data_free, &graph_edge_data_free, nullptr);
}

/* Creation routing model functions */

DijkstraZone::DijkstraZone(NetZone* father, std::string name, bool cached) : RoutedZone(father, name), cached_(cached)
{
}

void DijkstraZone::addRoute(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                            kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                            std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical)
{
  const char* srcName = src->getCname();
  const char* dstName = dst->getCname();

  addRouteCheckParams(src, dst, gw_src, gw_dst, link_list, symmetrical);

  /* Create the topology graph */
  if (not routeGraph_)
    routeGraph_ = xbt_graph_new_graph(1, nullptr);

  /* we don't check whether the route already exist, because the algorithm may find another path through some other
   * nodes */

  /* Add the route to the base */
  sg_platf_route_cbarg_t e_route = newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, 1);
  newRoute(src->id(), dst->id(), e_route);

  // Symmetrical YES
  if (symmetrical == true) {

    xbt_dynar_t nodes   = xbt_graph_get_nodes(routeGraph_);
    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src->id(), xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst->id(), xbt_node_t);
    xbt_edge_t edge     = xbt_graph_get_edge(routeGraph_, node_e_v, node_s_v);

    if (not gw_dst || not gw_src) {
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dstName, srcName);
      if (edge)
        THROWF(arg_error, 0, "Route from %s to %s already exists", dstName, srcName);
    } else {
      XBT_DEBUG("Load NetzoneRoute from %s@%s to %s@%s", dstName, gw_dst->getCname(), srcName, gw_src->getCname());
      if (edge)
        THROWF(arg_error, 0, "Route from %s@%s to %s@%s already exists", dstName, gw_dst->getCname(), srcName,
               gw_src->getCname());
    }

    if (gw_dst && gw_src) {
      NetPoint* gw_tmp = gw_src;
      gw_src           = gw_dst;
      gw_dst           = gw_tmp;
    }
    sg_platf_route_cbarg_t link_route_back =
        newExtendedRoute(hierarchy_, src, dst, gw_src, gw_dst, link_list, symmetrical, 0);
    newRoute(dst->id(), src->id(), link_route_back);
  }
}
}
}
} // namespace
