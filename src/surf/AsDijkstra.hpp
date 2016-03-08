/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_DIJKSTRA_HPP_
#define SURF_ROUTING_DIJKSTRA_HPP_

#include "src/surf/AsRoutedGraph.hpp"

typedef struct graph_node_data {
  int id;
  int graph_id;                 /* used for caching internal graph id's */
} s_graph_node_data_t, *graph_node_data_t;

typedef struct graph_node_map_element {
  xbt_node_t node;
} s_graph_node_map_element_t, *graph_node_map_element_t;

typedef struct route_cache_element {
  int *pred_arr;
  int size;
} s_route_cache_element_t, *route_cache_element_t;

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

/** Dijkstra routing data: fast initialization, slow lookup, small memory requirements, shortest path routing only */
class XBT_PRIVATE AsDijkstra : public AsRoutedGraph {
public:
  AsDijkstra(const char*name, bool cached);
  void Seal() override;

  ~AsDijkstra();
  xbt_node_t routeGraphNewNode(int id, int graph_id);
  graph_node_map_element_t nodeMapSearch(int id);
  void newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route);
    /**
     * For each vertex (node) already in the graph,
     * make sure it also has a loopback link; this loopback
     * can potentially already be in the graph, and in that
     * case nothing will be done.
     *
     * If no loopback is specified for a node, we will use
     * the loopback that is provided by the routing platform.
     *
     * After this function returns, any node in the graph
     * will have a loopback attached to it.
     */
  void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t route, double *lat) override;
  void addRoute(sg_platf_route_cbarg_t route) override;

  xbt_graph_t routeGraph_ = nullptr;     /* xbt_graph */
  xbt_dict_t graphNodeMap_ = nullptr;    /* map */
  xbt_dict_t routeCache_ = nullptr;      /* use in cache mode */
};

}
}

#endif /* SURF_ROUTING_DIJKSTRA_HPP_ */
