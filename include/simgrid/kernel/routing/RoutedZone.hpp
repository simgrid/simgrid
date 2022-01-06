/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_GENERIC_HPP_
#define SIMGRID_ROUTING_GENERIC_HPP_

#include <simgrid/kernel/routing/NetZoneImpl.hpp>

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing (abstract class)
 *
 * This abstract class factors code between its subclasses: Full, Dijkstra and Floyd.
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
  explicit RoutedZone(const std::string& name);

  void get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                 std::map<std::string, xbt_edge_t, std::less<>>* edges) override;

protected:
  Route* new_extended_route(RoutingMode hierarchy, NetPoint* gw_src, NetPoint* gw_dst,
                            const std::vector<resource::StandardLinkImpl*>& link_list, bool preserve_order);
  void get_route_check_params(const NetPoint* src, const NetPoint* dst) const;
  void add_route_check_params(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                              const std::vector<s4u::LinkInRoute>& link_list, bool symmetrical) const;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

XBT_PRIVATE xbt_node_t new_xbt_graph_node(const s_xbt_graph_t* graph, const char* name,
                                          std::map<std::string, xbt_node_t, std::less<>>* nodes);
XBT_PRIVATE xbt_edge_t new_xbt_graph_edge(const s_xbt_graph_t* graph, xbt_node_t s, xbt_node_t d,
                                          std::map<std::string, xbt_edge_t, std::less<>>* edges);

#endif /* SIMGRID_ROUTING_GENERIC_HPP_ */
