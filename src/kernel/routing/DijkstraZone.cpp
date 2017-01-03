/* Copyright (c) 2009-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/DijkstraZone.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/network_interface.hpp"

#include <float.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_dijkstra, surf, "Routing part of surf -- dijkstra routing logic");

/* Free functions */

static void route_cache_elem_free(void* e)
{
  route_cache_element_t elm = (route_cache_element_t)e;
  if (elm) {
    xbt_free(elm->pred_arr);
    xbt_free(elm);
  }
}

static void graph_node_map_elem_free(void* e)
{
  graph_node_map_element_t elm = (graph_node_map_element_t)e;
  xbt_free(elm);
}

static void graph_edge_data_free(void* e) // FIXME: useless code duplication
{
  sg_platf_route_cbarg_t e_route = (sg_platf_route_cbarg_t)e;
  if (e_route) {
    delete e_route->link_list;
    xbt_free(e_route);
  }
}

/* Utility functions */

namespace simgrid {
namespace kernel {
namespace routing {
void DijkstraZone::seal()
{
  xbt_node_t node = nullptr;
  unsigned int cursor2, cursor;

  /* Create the topology graph */
  if (!routeGraph_)
    routeGraph_ = xbt_graph_new_graph(1, nullptr);
  if (!graphNodeMap_)
    graphNodeMap_ = xbt_dict_new_homogeneous(&graph_node_map_elem_free);

  /* Add the loopback if needed */
  if (surf_network_model->loopback_ && hierarchy_ == RoutingMode::base) {
    xbt_dynar_foreach (xbt_graph_get_nodes(routeGraph_), cursor, node) {
      xbt_edge_t edge = nullptr;

      bool found = false;
      xbt_dynar_foreach (xbt_graph_node_get_outedges(node), cursor2, edge) {
        if (xbt_graph_edge_get_target(edge) == node) {
          found = true;
          break;
        }
      }

      if (!found) {
        sg_platf_route_cbarg_t e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->link_list             = new std::vector<Link*>();
        e_route->link_list->push_back(surf_network_model->loopback_);
        xbt_graph_new_edge(routeGraph_, node, node, e_route);
      }
    }
  }

  /* initialize graph indexes in nodes after graph has been built */
  xbt_dynar_t nodes = xbt_graph_get_nodes(routeGraph_);

  xbt_dynar_foreach (nodes, cursor, node) {
    graph_node_data_t data = (graph_node_data_t)xbt_graph_node_get_data(node);
    data->graph_id         = cursor;
  }
}

xbt_node_t DijkstraZone::routeGraphNewNode(int id, int graph_id)
{
  xbt_node_t node              = nullptr;
  graph_node_data_t data       = nullptr;
  graph_node_map_element_t elm = nullptr;

  data           = xbt_new0(struct graph_node_data, 1);
  data->id       = id;
  data->graph_id = graph_id;
  node           = xbt_graph_new_node(routeGraph_, data);

  elm       = xbt_new0(struct graph_node_map_element, 1);
  elm->node = node;
  xbt_dict_set_ext(graphNodeMap_, (char*)(&id), sizeof(int), (xbt_dictelm_t)elm, nullptr);

  return node;
}

graph_node_map_element_t DijkstraZone::nodeMapSearch(int id)
{
  return (graph_node_map_element_t)xbt_dict_get_or_null_ext(graphNodeMap_, (char*)(&id), sizeof(int));
}

/* Parsing */

void DijkstraZone::newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route)
{
  XBT_DEBUG("Load Route from \"%d\" to \"%d\"", src_id, dst_id);
  xbt_node_t src = nullptr;
  xbt_node_t dst = nullptr;

  graph_node_map_element_t src_elm = nodeMapSearch(src_id);
  graph_node_map_element_t dst_elm = nodeMapSearch(dst_id);

  if (src_elm)
    src = src_elm->node;

  if (dst_elm)
    dst = dst_elm->node;

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

void DijkstraZone::getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t route, double* lat)
{
  getRouteCheckParams(src, dst);
  int src_id = src->id();
  int dst_id = dst->id();

  int* pred_arr     = nullptr;
  int size          = 0;
  xbt_dynar_t nodes = xbt_graph_get_nodes(routeGraph_);

  /* Use the graph_node id mapping set to quickly find the nodes */
  graph_node_map_element_t src_elm = nodeMapSearch(src_id);
  graph_node_map_element_t dst_elm = nodeMapSearch(dst_id);

  int src_node_id = ((graph_node_data_t)xbt_graph_node_get_data(src_elm->node))->graph_id;
  int dst_node_id = ((graph_node_data_t)xbt_graph_node_get_data(dst_elm->node))->graph_id;

  /* if the src and dst are the same */
  if (src_node_id == dst_node_id) {

    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    xbt_edge_t edge     = xbt_graph_get_edge(routeGraph_, node_s_v, node_e_v);

    if (edge == nullptr)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->name().c_str(), dst->name().c_str());

    sg_platf_route_cbarg_t e_route = (sg_platf_route_cbarg_t)xbt_graph_edge_get_data(edge);

    for (auto link : *e_route->link_list) {
      route->link_list->insert(route->link_list->begin(), link);
      if (lat)
        *lat += static_cast<Link*>(link)->latency();
    }
  }

  route_cache_element_t elm = nullptr;
  if (routeCache_) { /* cache mode  */
    elm = (route_cache_element_t)xbt_dict_get_or_null_ext(routeCache_, (char*)(&src_id), sizeof(int));
  }

  if (elm) { /* cached mode and cache hit */
    pred_arr = elm->pred_arr;
  } else { /* not cached mode, or cache miss */

    int nr_nodes      = xbt_dynar_length(nodes);
    double* cost_arr  = xbt_new0(double, nr_nodes); /* link cost from src to other hosts */
    pred_arr          = xbt_new0(int, nr_nodes);    /* predecessors in path from src */
    xbt_heap_t pqueue = xbt_heap_new(nr_nodes, xbt_free_f);

    /* initialize */
    cost_arr[src_node_id] = 0.0;

    for (int i = 0; i < nr_nodes; i++) {
      if (i != src_node_id) {
        cost_arr[i] = DBL_MAX;
      }

      pred_arr[i] = 0;

      /* initialize priority queue */
      int* nodeid = xbt_new0(int, 1);
      *nodeid     = i;
      xbt_heap_push(pqueue, nodeid, cost_arr[i]);
    }

    /* apply dijkstra using the indexes from the graph's node array */
    while (xbt_heap_size(pqueue) > 0) {
      int* v_id         = (int*)xbt_heap_pop(pqueue);
      xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
      xbt_edge_t edge   = nullptr;
      unsigned int cursor;

      xbt_dynar_foreach (xbt_graph_node_get_outedges(v_node), cursor, edge) {
        xbt_node_t u_node                  = xbt_graph_edge_get_target(edge);
        graph_node_data_t data             = (graph_node_data_t)xbt_graph_node_get_data(u_node);
        int u_id                           = data->graph_id;
        sg_platf_route_cbarg_t tmp_e_route = (sg_platf_route_cbarg_t)xbt_graph_edge_get_data(edge);
        int cost_v_u                       = tmp_e_route->link_list->size(); /* count of links, old model assume 1 */

        if (cost_v_u + cost_arr[*v_id] < cost_arr[u_id]) {
          pred_arr[u_id] = *v_id;
          cost_arr[u_id] = cost_v_u + cost_arr[*v_id];
          int* nodeid    = xbt_new0(int, 1);
          *nodeid        = u_id;
          xbt_heap_push(pqueue, nodeid, cost_arr[u_id]);
        }
      }

      /* free item popped from pqueue */
      xbt_free(v_id);
    }

    xbt_free(cost_arr);
    xbt_heap_free(pqueue);
  }

  /* compose route path with links */
  NetCard *gw_src = nullptr, *gw_dst, *prev_gw_src, *first_gw = nullptr;
  NetCard *gw_dst_net_elm = nullptr, *prev_gw_src_net_elm = nullptr;

  for (int v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v = xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v      = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge        = xbt_graph_get_edge(routeGraph_, node_pred_v, node_v);

    if (edge == nullptr)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->name().c_str(), dst->name().c_str());

    prev_gw_src = gw_src;

    sg_platf_route_cbarg_t e_route = (sg_platf_route_cbarg_t)xbt_graph_edge_get_data(edge);
    gw_src                         = e_route->gw_src;
    gw_dst                         = e_route->gw_dst;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (hierarchy_ == RoutingMode::recursive && v != dst_node_id &&
        strcmp(gw_dst->name().c_str(), prev_gw_src->name().c_str())) {
      std::vector<Link*>* e_route_as_to_as = new std::vector<Link*>();

      getGlobalRoute(gw_dst_net_elm, prev_gw_src_net_elm, e_route_as_to_as, nullptr);
      auto pos = route->link_list->begin();
      for (auto link : *e_route_as_to_as) {
        route->link_list->insert(pos, link);
        if (lat)
          *lat += link->latency();
        pos++;
      }
    }

    for (auto link : *e_route->link_list) {
      route->link_list->insert(route->link_list->begin(), link);
      if (lat)
        *lat += static_cast<Link*>(link)->latency();
    }
    size++;
  }

  if (hierarchy_ == RoutingMode::recursive) {
    route->gw_src = gw_src;
    route->gw_dst = first_gw;
  }

  if (routeCache_ && elm == nullptr) {
    /* add to predecessor list of the current src-host to cache */
    elm           = xbt_new0(struct route_cache_element, 1);
    elm->pred_arr = pred_arr;
    elm->size     = size;
    xbt_dict_set_ext(routeCache_, (char*)(&src_id), sizeof(int), (xbt_dictelm_t)elm, nullptr);
  }

  if (!routeCache_)
    xbt_free(pred_arr);
}

DijkstraZone::~DijkstraZone()
{
  xbt_graph_free_graph(routeGraph_, &xbt_free_f, &graph_edge_data_free, &xbt_free_f);
  xbt_dict_free(&graphNodeMap_);
  xbt_dict_free(&routeCache_);
}

/* Creation routing model functions */

DijkstraZone::DijkstraZone(NetZone* father, const char* name, bool cached) : RoutedZone(father, name)
{
  if (cached)
    routeCache_ = xbt_dict_new_homogeneous(&route_cache_elem_free);
}

void DijkstraZone::addRoute(sg_platf_route_cbarg_t route)
{
  NetCard* src        = route->src;
  NetCard* dst        = route->dst;
  const char* srcName = src->name().c_str();
  const char* dstName = dst->name().c_str();

  addRouteCheckParams(route);

  /* Create the topology graph */
  if (!routeGraph_)
    routeGraph_ = xbt_graph_new_graph(1, nullptr);
  if (!graphNodeMap_)
    graphNodeMap_ = xbt_dict_new_homogeneous(&graph_node_map_elem_free);

  /* we don't check whether the route already exist, because the algorithm may find another path through some other
   * nodes */

  /* Add the route to the base */
  sg_platf_route_cbarg_t e_route = newExtendedRoute(hierarchy_, route, 1);
  newRoute(src->id(), dst->id(), e_route);

  // Symmetrical YES
  if (route->symmetrical == true) {
    if (!route->gw_dst && !route->gw_src)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dstName, srcName);
    else
      XBT_DEBUG("Load NetzoneRoute from %s@%s to %s@%s", dstName, route->gw_dst->name().c_str(), srcName,
                route->gw_src->name().c_str());

    xbt_dynar_t nodes   = xbt_graph_get_nodes(routeGraph_);
    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src->id(), xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst->id(), xbt_node_t);
    xbt_edge_t edge     = xbt_graph_get_edge(routeGraph_, node_e_v, node_s_v);

    if (edge)
      THROWF(arg_error, 0, "Route from %s@%s to %s@%s already exists", dstName, route->gw_dst->name().c_str(), srcName,
             route->gw_src->name().c_str());

    if (route->gw_dst && route->gw_src) {
      NetCard* gw_tmp = route->gw_src;
      route->gw_src   = route->gw_dst;
      route->gw_dst   = gw_tmp;
    }
    sg_platf_route_cbarg_t link_route_back = newExtendedRoute(hierarchy_, route, 0);
    newRoute(dst->id(), src->id(), link_route_back);
  }
}
}
}
} // namespace
