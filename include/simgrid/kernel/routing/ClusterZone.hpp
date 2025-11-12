/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_CLUSTER_HPP_
#define SIMGRID_ROUTING_CLUSTER_HPP_

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <xbt/ex.h>

#include <numeric>
#include <unordered_map>

namespace simgrid::kernel::routing {

/**
 * @brief Placeholder for old ClusterZone class
 *
 * The ClusterZone is now implemented through a StarZone.
 *
 * Leave this class as a placeholder to avoid compatibility issues
 * with codes that use the Engine::get_filtered_netzones.
 *
 * The old ClusterZone is now called BaseCluster and it's used ONLY as base to
 * implement the complex cluster such as Torus, DragonFly and Fat-Tree
 */
class ClusterZone : public NetZoneImpl {
protected:
  using NetZoneImpl::NetZoneImpl;
};

/**
 *  @brief Old ClusterZone implementation
 *
 *  NOTE: Cannot be directly instantiated anymore.
 *
 *  NetZone where each component is connected through a private link
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
class XBT_PRIVATE ClusterBase : public ClusterZone {
  friend s4u::NetZone;
  /* We use a map instead of a std::vector here because that's a sparse vector. Some values may not exist */
  /* The pair is {link_up, link_down} */
  std::unordered_map<unsigned long, std::pair<resource::StandardLinkImpl*, resource::StandardLinkImpl*>> private_links_;
  std::unordered_map<unsigned long, NetPoint*> gateways_; //!< list of gateways for leafs (if they're netzones)
  resource::StandardLinkImpl* backbone_ = nullptr;
  NetPoint* router_                     = nullptr;
  bool has_limiter_                     = false;
  bool has_loopback_                    = false;
  int tot_elements_                     = 0;
  std::vector<unsigned long> dims_;
  // Callbacks
  /**
   * @brief Callback used to set the Host located at some leaf of clusters (Torus, FatTree, etc)
   *
   * @param zone: The parent zone, needed for creating new resources (hosts, links)
   * @param coord: the coordinates of the element
   * @param id: Internal identifier of the element
   * @return Host*: returns newly created host
   */
  std::function<s4u::Host*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)> host_cb_ =
      {};
  /**
   * @brief Callback used to set the NetZone located at some leaf of clusters (Torus, FatTree, etc)
   *
   * @param zone: The parent zone, needed for creating new resources (hosts, links)
   * @param coord: the coordinates of the element
   * @param id: Internal identifier of the element
   * @return NetZone*: returns newly created netzone
   */
  std::function<s4u::NetZone*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>
      netzone_cb_ = {};
  /**
   * @brief Callbacks used to set the loopback and limiter links for some leaf of the cluster (Torus, FatTree, etc)
   *
   * The coord parameter depends on the cluster being created:
   * - Torus: Direct translation of the Torus' dimensions, e.g. (0, 0, 0) for a 3-D Torus
   * - Fat-Tree: A pair (level in the tree, id), e.g. (0, 0): first leaf and (1,0): first switch at level 1.
   * - Dragonfly: a tuple (group, chassis, blades/routers, nodes), e.g. (0, 0, 0, 0) for first node in the cluster.
   * Important: To identify the router inside a "group, chassis, blade", we use MAX_UINT in the last parameter (e.g. 0,
   * 0, 0, 4294967295).
   *
   * @param zone: The newly create zone, needed for creating new resources (hosts, links)
   * @param coord: the coordinates of the element
   * @param id: Internal identifier of the element
   * @return Pointer to the Link
   */
  std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)> limiter_cb_ =
      {};
  std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>
      loopback_cb_ = {};

  unsigned long num_links_per_node_ = 1; /* may be 1 (if only a private link), 2 or 3 (if limiter and loopback) */

  s4u::Link::SharingPolicy link_sharing_policy_ =
      s4u::Link::SharingPolicy::SPLITDUPLEX; //!< cluster links: sharing policy
  double link_bw_  = 0.0;                    //!< cluster links: bandwidth
  double link_lat_ = 0.0;                    //!< cluster links: latency

protected:
  using ClusterZone::ClusterZone;
  void set_cluster_dimensions(const std::vector<unsigned long>& dimensions) { dims_ = dimensions; }
  void set_num_links_per_node(unsigned long num) { num_links_per_node_ = num; }
  resource::StandardLinkImpl* get_uplink_from(unsigned long position) const
  {
    return private_links_.at(position).first;
  }
  resource::StandardLinkImpl* get_downlink_to(unsigned long position) const
  {
    return private_links_.at(position).second;
  }

  double get_link_latency() const { return link_lat_; }
  double get_link_bandwidth() const { return link_bw_; }
  
  void set_tot_elements(int tot) { tot_elements_ = tot;}
  int get_tot_elements() const { return tot_elements_; }
  s4u::Link::SharingPolicy get_link_sharing_policy() const { return link_sharing_policy_; }

  void set_loopback();
  bool has_loopback() const { return has_loopback_; }
  void set_limiter();
  bool has_limiter() const { return has_limiter_; }

  void set_host_cb(const std::function<s4u::Host*(s4u::NetZone* zone, const std::vector<unsigned long>& coord,
                                                  unsigned long id)>& cb)
  {
    host_cb_ = cb;
  }
  const std::function<s4u::Host*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>&
  get_host_cb() const
  {
    return host_cb_;
  }
  void set_netzone_cb(const std::function<s4u::NetZone*(s4u::NetZone* zone, const std::vector<unsigned long>& coord,
                                                        unsigned long id)>& cb)
  {
    netzone_cb_ = cb;
  }
  const std::function<s4u::NetZone*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>&
  get_netzone_cb() const
  {
    return netzone_cb_;
  }
  void set_loopback_cb(const std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord,
                                                      unsigned long id)>& cb)
  {
    loopback_cb_ = cb;
  }
  const std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>&
  get_loopback_cb() const
  {
    return loopback_cb_;
  }
  void set_limiter_cb(const std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord,
                                                     unsigned long id)>& cb)
  {
    limiter_cb_ = cb;
  }
  const std::function<s4u::Link*(s4u::NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>&
  get_limiter_cb() const
  {
    return limiter_cb_;
  }

  void set_backbone(resource::StandardLinkImpl* bb) { backbone_ = bb; }
  bool has_backbone() const { return backbone_ != nullptr; }

  void set_router(NetPoint* router) { router_ = router; }
  /** @brief Sets gateway for the leaf */
  void set_gateway(unsigned long position, NetPoint* gateway);
  /** @brief Gets gateway for the leaf or nullptr */
  NetPoint* get_gateway(unsigned long position);
  void add_private_link_at(unsigned long position,
                           std::pair<resource::StandardLinkImpl*, resource::StandardLinkImpl*> link);
  bool private_link_exists_at(unsigned long position) const
  {
    return private_links_.find(position) != private_links_.end();
  }

  unsigned long node_pos(unsigned long id) const { return id * num_links_per_node_; }
  unsigned long node_pos_with_loopback(unsigned long id) const { return node_pos(id) + (has_loopback_ ? 1 : 0); }

public:
  /** Fill the leaf retrieving netpoint from a user's callback */
  std::tuple<NetPoint*, s4u::Link*, s4u::Link*> fill_leaf_from_cb(unsigned long position);

  /** @brief Set the characteristics of links inside a Cluster zone */
  virtual void set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy);
  unsigned long node_pos_with_loopback_limiter(unsigned long id) const
  {
    return node_pos_with_loopback(id) + (has_limiter_ ? 1 : 0);
  }
};
} // namespace simgrid::kernel::routing

#endif /* SIMGRID_ROUTING_CLUSTER_HPP_ */
