/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_CLUSTER_HPP_
#define SIMGRID_ROUTING_CLUSTER_HPP_

#include <simgrid/kernel/routing/NetZoneImpl.hpp>

#include <unordered_map>

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone where each component is connected through a private link
 *
 *  Cluster zones have a collection of private links that interconnect their components.
 *  This is particularly well adapted to model a collection of elements interconnected
 *  through a hub or a through a switch.
 *
 *  In a cluster, each component are given from 1 to 3 private links at creation:
 *   - Private link (mandatory): link connecting the component to the cluster core.
 *   - Limiter (optional): Additional link on the way from the component to the cluster core
 *   - Loopback (optional): non-shared link connecting the component to itself.
 *
 *  Then, the cluster core may be constituted of a specific backbone link or not;
 *  A backbone can easily represent a network connected in a Hub.
 *  If you want to model a switch, either don't use a backbone at all,
 *  or use a fatpipe link (non-shared link) to represent the switch backplane.
 *
 *  \verbatim
 *   (outer world)
 *         |
 *   ======+====== <--backbone
 *   |   |   |   |
 * l0| l1| l2| l4| <-- private links + limiters
 *   |   |   |   |
 *   X   X   X   X <-- cluster's hosts
 * \endverbatim
 *
 * \verbatim
 *   (outer world)
 *         |       <-- NO backbone
 *        /|\
 *       / | \     <-- private links + limiters     __________
 *      /  |  \
 *  l0 / l1|   \l2
 *    /    |    \
 * host0 host1 host2
 * \endverbatim

 *  So, a communication from a host A to a host B goes through the following links (if they exist):
 *   <tt>limiter(A)_UP, private(A)_UP, backbone, private(B)_DOWN, limiter(B)_DOWN.</tt>
 *  link_UP and link_DOWN usually share the exact same characteristics, but their
 *  performance are not shared, to model the fact that TCP links are full-duplex.
 *
 *  A cluster is connected to the outer world through a router that is connected
 *  directly to the cluster's backbone (no private link).
 *
 *  A communication from a host A to the outer world goes through the following links:
 *   <tt>limiter(A)_UP, private(A)_UP, backbone</tt>
 *  (because the private router is directly connected to the cluster core).
 */

class ClusterZone : public NetZoneImpl {
public:
  explicit ClusterZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel);

  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency) override;
  void get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                 std::map<std::string, xbt_edge_t, std::less<>>* edges) override;

  virtual void create_links_for_node(ClusterCreationArgs* cluster, int id, int rank, unsigned int position);
  virtual void parse_specific_arguments(ClusterCreationArgs*)
  {
    /* this routing method does not require any specific argument */
  }

  /* We use a map instead of a std::vector here because that's a sparse vector. Some values may not exist */
  /* The pair is {link_up, link_down} */
  std::unordered_map<unsigned int, std::pair<kernel::resource::LinkImpl*, kernel::resource::LinkImpl*>> private_links_;

  unsigned int node_pos(int id) const { return id * num_links_per_node_; }
  unsigned int node_pos_with_loopback(int id) const { return node_pos(id) + (has_loopback_ ? 1 : 0); }
  unsigned int node_pos_with_loopback_limiter(int id) const
  {
    return node_pos_with_loopback(id) + (has_limiter_ ? 1 : 0);
  }

  kernel::resource::LinkImpl* backbone_ = nullptr;
  NetPoint* router_              = nullptr;
  bool has_limiter_                = false;
  bool has_loopback_               = false;
  unsigned int num_links_per_node_ = 1; /* may be 1 (if only a private link), 2 or 3 (if limiter and loopback) */
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_ROUTING_CLUSTER_HPP_ */
