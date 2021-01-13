/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_DIJKSTRA_HPP_
#define SURF_ROUTING_DIJKSTRA_HPP_

#include <simgrid/kernel/routing/RoutedZone.hpp>


namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing computed on need with Dijkstra
 *
 *  The path between components is computed each time you request it,
 *  using the Dijkstra algorithm. A cache can be used to reduce the computation.
 *
 *  This result in rather small platform file, very fast initialization, and very low memory requirements, but somehow
 * long path resolution times.
 */
class XBT_PRIVATE DijkstraZone : public RoutedZone {
private:
  static void route_graph_delete(xbt_graph_t);

  std::unique_ptr<s_xbt_graph_t, decltype(&DijkstraZone::route_graph_delete)> route_graph_{
      xbt_graph_new_graph(1, nullptr), &DijkstraZone::route_graph_delete};
  std::map<int, xbt_node_t> graph_node_map_;
  bool cached_;
  std::map<int, std::vector<int>> route_cache_;

  xbt_node_t route_graph_new_node(int id);
  xbt_node_t node_map_search(int id);
  void new_edge(int src_id, int dst_id, RouteCreationArgs* e_route);

public:
  DijkstraZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel, bool cached);

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
  void seal() override;
  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat) override;
  void add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                 std::vector<resource::LinkImpl*>& link_list, bool symmetrical) override;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SURF_ROUTING_DIJKSTRA_HPP_ */
