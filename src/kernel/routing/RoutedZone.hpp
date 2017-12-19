/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_GENERIC_HPP_
#define SIMGRID_ROUTING_GENERIC_HPP_

#include "src/kernel/routing/NetZoneImpl.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing (abstract class)
 *
 * This abstract class factorizes code between its subclasses: Full, Dijkstra and Floyd.
 *
 * <table>
 * <caption>Comparison of the RoutedZone subclasses</caption>
 * <tr><td></td><td>DijkstraZone</td><td>FloydZone</td><td>FullZone</td></tr>
 * <tr><td><b>Platform-file content</b></td>
 * <td>Only 1-hop routes (rather small)</td>
 * <td>Only 1-hop routes (rather small)</td>
 * <td>Every path, explicitly (very large)</td>
 * </tr>
 * <tr><td><b>Initialization time</b></td>
 * <td>Almost nothing</td>
 * <td>Floyd-Warshall algorithm: O(n^3)</td>
 * <td>Almost nothing</td>
 * </tr>
 * <tr><td><b>Memory usage</b></td>
 * <td>1-hop routes (+ cache of routes)</td>
 * <td>O(n^2) data (intermediate)</td>
 * <td>O(n^2) + sum of path lengths (very large)</td>
 * </tr>
 * <tr><td><b>Lookup time</b></td>
 * <td>Dijkstra Algo: O(n^3)</td>
 * <td>not much (reconstruction phase)</td>
 * <td>Almost nothing</td>
 * </tr>
 * <tr><td><b>Expressiveness</b></td>
 * <td>Only shortest path</td>
 * <td>Only shortest path</td>
 * <td>Everything</td>
 * </tr>
 * </table>
 */

class XBT_PRIVATE RoutedZone : public NetZoneImpl {
public:
  explicit RoutedZone(NetZone* father, std::string name);

  void getGraph(xbt_graph_t graph, std::map<std::string, xbt_node_t>* nodes,
                std::map<std::string, xbt_edge_t>* edges) override;
  virtual sg_platf_route_cbarg_t newExtendedRoute(RoutingMode hierarchy, NetPoint* src, NetPoint* dst, NetPoint* gw_src,
                                                  NetPoint* gw_dst, std::vector<simgrid::surf::LinkImpl*>& link_list,
                                                  bool symmetrical, bool change_order);

protected:
  void getRouteCheckParams(NetPoint* src, NetPoint* dst);
  void addRouteCheckParams(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                           kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                           std::vector<simgrid::surf::LinkImpl*>& link_list, bool symmetrical);
};
}
}
} // namespace

extern "C" {
XBT_PRIVATE xbt_node_t new_xbt_graph_node(xbt_graph_t graph, const char* name,
                                          std::map<std::string, xbt_node_t>* nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge(xbt_graph_t graph, xbt_node_t s, xbt_node_t d,
                                          std::map<std::string, xbt_edge_t>* edges);
}

#endif /* SIMGRID_ROUTING_GENERIC_HPP_ */
