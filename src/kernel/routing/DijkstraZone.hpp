/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_DIJKSTRA_HPP_
#define SURF_ROUTING_DIJKSTRA_HPP_

#include "src/kernel/routing/RoutedZone.hpp"

struct s_graph_node_data_t {
  int id;
  int graph_id; /* used for caching internal graph id's */
};
typedef s_graph_node_data_t* graph_node_data_t;

namespace simgrid {
namespace kernel {
namespace routing {

/***********
 * Classes *
 ***********/

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing computed on need with Dijsktra
 *
 *  The path between components is computed each time you request it,
 *  using the Dijkstra algorithm. A cache can be used to reduce the computation.
 *
 *  This result in rather small platform file, very fast initialization, and very low memory requirements, but somehow long path resolution times.
 */
class XBT_PRIVATE DijkstraZone : public RoutedZone {
public:
  DijkstraZone(NetZone* father, std::string name, bool cached);
  void seal() override;

  ~DijkstraZone() override;
  xbt_node_t routeGraphNewNode(int id, int graph_id);
  xbt_node_t nodeMapSearch(int id);
  void newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route);
  /* For each vertex (node) already in the graph,
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
  void getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* lat) override;
  void addRoute(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical) override;

  xbt_graph_t routeGraph_  = nullptr; /* xbt_graph */
  std::map<int, xbt_node_t> graphNodeMap_;     /* map */
  bool cached_;                                /* cache mode */
  std::map<int, std::vector<int>> routeCache_; /* use in cache mode */
};
}
}
} // namespaces

#endif /* SURF_ROUTING_DIJKSTRA_HPP_ */
