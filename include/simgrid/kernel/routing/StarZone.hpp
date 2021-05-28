/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ROUTING_STARZONE_HPP_
#define SIMGRID_KERNEL_ROUTING_STARZONE_HPP_

#include <simgrid/kernel/routing/ClusterZone.hpp>

#include <unordered_map>
#include <unordered_set>

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone where components are connected following a star topology
 *
 *  Star zones have a collection of private links that interconnect their components.
 *  By default, all components inside the star zone are interconnected with no links.
 *
 *  You can use add_route to set the links to be used during communications, 3
 *  configurations are possible:
 *  - (*(all) -> Component): links used in the outgoing communications from component (UP).
 *  - (Component -> *(all)): links used in ine ingoing communication to component (DOWN).
 *  - Loopback: links connecting the component to itself.
 *
 *  @note: Communications between nodes inside the Star zone cannot have duplicate links.
 *         All duplicated links are automatically removed when building the route.
 *
 * \verbatim
 *   (outer world)
 *         |
 *     l3 /|\ l4
 *       / | \     <-- links
 *      +  |  +
 *  l0 / l1|   \l2
 *    /    |    \
 *   A     B     C <-- netpoints
 * \endverbatim
 *
 *  So, a communication from the host A to the host B goes through the following links:
 *   <tt>l0, l3, l1.</tt>
 *
 *  In the same way, a communication from host A to nodes outside this netzone will
 *  use the same links <tt> l0, l3. </tt>
 *
 *  \verbatim
 *   (outer world)
 *         |
 *   ======+====== <-- backbone
 *   |   |   |   |
 * l0| l1| l2| l4| <-- links
 *   |   |   |   |
 *   A   B   C   D <-- netpoints
 * \endverbatim
 *
 *  In this case, a communication from A to B goes through the links: <tt> l0, backbone, l1. </tt>
 *  Note that the backbone only appears once in the link list.
 */
class StarZone : public ClusterZone { // implements the old ClusterZone
public:
  explicit StarZone(const std::string& name);

  void get_local_route(NetPoint* src, NetPoint* dst, Route* route, double* latency) override;
  void get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                 std::map<std::string, xbt_edge_t, std::less<>>* edges) override;

  void add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                 const std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical) override;
  void do_seal() override;

private:
  class StarRoute {
  public:
    std::vector<resource::LinkImpl*> links_up;   //!< list of links UP for route (can be empty)
    std::vector<resource::LinkImpl*> links_down; //!< list of links DOWN for route (can be empty)
    std::vector<resource::LinkImpl*> loopback;   //!< loopback links, cannot be empty if configured
    bool links_up_set   = false;                 //!< bool to indicate that links_up was configured (empty or not)
    bool links_down_set = false;                 //!< same for links_down
    NetPoint* gateway   = nullptr;
    bool has_loopback() const { return not loopback.empty(); }
    bool has_links_up() const { return links_up_set; }
    bool has_links_down() const { return links_down_set; }
  };
  /** @brief Auxiliary method to add links to a route */
  void add_links_to_route(const std::vector<resource::LinkImpl*>& links, Route* route, double* latency,
                          std::unordered_set<resource::LinkImpl*>& added_links) const;
  /** @brief Auxiliary methods to check params received in add_route method */
  void check_add_route_param(const NetPoint* src, const NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                             bool symmetrical) const;
  std::unordered_map<unsigned int, StarRoute> routes_;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ROUTING_STARZONE_HPP_ */
