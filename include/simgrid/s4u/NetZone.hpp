/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_NETZONE_HPP
#define SIMGRID_S4U_NETZONE_HPP

#include "simgrid/s4u/Host.hpp"
#include "xbt/base.h"
#include <simgrid/forward.h>
#include <simgrid/s4u/Link.hpp>
#include <xbt/graph.h>
#include <xbt/signal.hpp>

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace simgrid::s4u {
// Extra data structure for complex constructors

#ifndef DOXYGEN
struct ClusterCallbacks { // XBT_ATTRIB_DEPRECATED_v403
  using ClusterNetPointCb = std::pair<kernel::routing::NetPoint*, kernel::routing::NetPoint*>(
      NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id);

  using ClusterNetZoneCb = NetZone*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id);
  using ClusterHostCb = Host*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id);
  using ClusterLinkCb = Link*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id);

  bool by_netzone_ = false;
  bool is_by_netzone() const { return by_netzone_; }
  bool by_netpoint_ = false;                           // XBT_ATTRIB_DEPRECATED_v403
  bool is_by_netpoint() const { return by_netpoint_; } // XBT_ATTRIB_DEPRECATED_v403
  std::function<ClusterNetPointCb> netpoint;           // XBT_ATTRIB_DEPRECATED_v403
  std::function<ClusterHostCb> host;
  std::function<ClusterNetZoneCb> netzone;
  std::function<ClusterLinkCb> loopback = {};
  std::function<ClusterLinkCb> limiter  = {};
  explicit ClusterCallbacks(const std::function<ClusterNetZoneCb>& set_netzone)
      : by_netzone_(true), netzone(set_netzone) { /* nothing to do */ };

  ClusterCallbacks(const std::function<ClusterNetZoneCb>& set_netzone, const std::function<ClusterLinkCb>& set_loopback,
                   const std::function<ClusterLinkCb>& set_limiter)
      : by_netzone_(true), netzone(set_netzone), loopback(set_loopback), limiter(set_limiter) { /* nothing to do */ };

  explicit ClusterCallbacks(const std::function<ClusterHostCb>& set_host) : host(set_host) { /* nothing to do */ };

  ClusterCallbacks(const std::function<ClusterHostCb>& set_host, const std::function<ClusterLinkCb>& set_loopback,
                   const std::function<ClusterLinkCb>& set_limiter)
      : host(set_host), loopback(set_loopback), limiter(set_limiter) { /* nothing to do */ };

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403(
      "Please use callback with either a Host/NetZone creation function as first "
      "parameter") explicit ClusterCallbacks(const std::function<ClusterNetPointCb>& set_netpoint)
      : by_netpoint_(true), netpoint(set_netpoint) { /* nothing to do */ };
  XBT_ATTRIB_DEPRECATED_v403("Please use callback with either a Host/NetZone creation function as first parameter")
      ClusterCallbacks(const std::function<ClusterNetPointCb>& set_netpoint,
                       const std::function<ClusterLinkCb>& set_loopback,
                       const std::function<ClusterLinkCb>& set_limiter)
      : by_netpoint_(true)
      , netpoint(set_netpoint)
      , loopback(set_loopback)
      , limiter(set_limiter) { /* nothing to do */ };
#endif
};

struct XBT_PUBLIC FatTreeParams { // XBT_ATTRIB_DEPRECATED_v403
  unsigned int levels;
  std::vector<unsigned int> down;
  std::vector<unsigned int> up;
  std::vector<unsigned int> count;
  FatTreeParams(unsigned int n_levels, const std::vector<unsigned int>& down_links,
                const std::vector<unsigned int>& up_links, const std::vector<unsigned int>& link_counts)
      : levels(n_levels), down(down_links), up(up_links), count(link_counts)
  {
  }
};

struct XBT_PUBLIC DragonflyParams { // XBT_ATTRIB_DEPRECATED_v403
  std::pair<unsigned int, unsigned int> groups;
  std::pair<unsigned int, unsigned int> chassis;
  std::pair<unsigned int, unsigned int> routers;
  unsigned int nodes;
  DragonflyParams(const std::pair<unsigned int, unsigned int>& groups,
                  const std::pair<unsigned int, unsigned int>& chassis,
                  const std::pair<unsigned int, unsigned int>& routers, unsigned int nodes)
      : groups(groups), chassis(chassis), routers(routers), nodes(nodes) {}
};
#endif

/** @brief Networking Zones
 *
 * A netzone is a network container, in charge of routing information between elements (hosts) and to the nearby
 * netzones. In SimGrid, there is a hierarchy of netzones, with a unique root zone (that you can retrieve from the
 * s4u::Engine).
 */
class XBT_PUBLIC NetZone {
#ifndef DOXYGEN
  friend kernel::routing::NetZoneImpl;
#endif

  kernel::routing::NetZoneImpl* const pimpl_ = nullptr;

protected:
  explicit NetZone(kernel::routing::NetZoneImpl* impl) : pimpl_(impl) {}

public:
  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that netzone as a C string */
  const char* get_cname() const;

  kernel::routing::NetZoneImpl* get_pimpl() { return pimpl_; }
  NetZone* get_parent() const;
#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403("Please stop using NetZone::set_parent(). It is useless with the new platform API.")
      NetZone* set_parent(const NetZone* parent);
#endif
  std::vector<NetZone*> get_children() const;

  std::vector<Host*> get_all_hosts() const;
  size_t get_host_count() const;

  kernel::routing::NetZoneImpl* get_impl() const { return pimpl_; }

  /** Get the properties assigned to a netzone */
  const std::unordered_map<std::string, std::string>* get_properties() const;
  /** Set the properties assigned to a netzone */
  NetZone* set_properties(const std::unordered_map<std::string, std::string>& properties);
  /** Retrieve the property value (or nullptr if not set) */
  const char* get_property(const std::string& key) const;
  void set_property(const std::string& key, const std::string& value);
  /** @brief Get the netpoint associated to this netzone */
  kernel::routing::NetPoint* get_netpoint() const;
  /** @brief Get the gateway associated to this netzone */
  kernel::routing::NetPoint* get_gateway() const;
  kernel::routing::NetPoint* get_gateway(const std::string& name) const;
  void set_gateway(const s4u::Host* router) { set_gateway(router->get_netpoint()); }
  void set_gateway(kernel::routing::NetPoint* router);
  void set_gateway(const std::string& name, kernel::routing::NetPoint* router);

  void extract_xbt_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                         std::map<std::string, xbt_edge_t, std::less<>>* edges);

  /* Add content to the netzone, at parsing time. It should be sealed afterward. */
  unsigned long add_component(kernel::routing::NetPoint* elm); /* A host, a router or a netzone, whatever */

  /**
   * @brief Add a route between 2 netzones, and same in other direction
   * @param src Source netzone
   * @param dst Destination netzone
   * @param links List of links
   */
  void add_route(const NetZone* src, const NetZone* dst, const std::vector<const Link*>& links);

  /**
   * @brief Add a route between 2 netzones, and same in other direction
   * @param src Source netzone
   * @param dst Destination netzone
   * @param link_list List of links and their direction used in this communication
   * @param symmetrical Bi-directional communication
   */
  void add_route(const NetZone* src, const NetZone* dst, const std::vector<LinkInRoute>& link_list,
                 bool symmetrical = true);

#ifndef DOXYGEN
  /**
   * @brief Add a route between 2 netpoints
   *
   * Create a route:
   * - route between 2 hosts/routers in same netzone, no gateway is needed
   * - route between 2 netzones, connecting 2 gateways.
   *
   * @param src Source netzone's netpoint
   * @param dst Destination netzone' netpoint
   * @param gw_src Netpoint of the gateway in the source netzone
   * @param gw_dst Netpoint of the gateway in the destination netzone
   * @param link_list List of links and their direction used in this communication
   * @param symmetrical Bi-directional communication
   */
  XBT_ATTRIB_DEPRECATED_v403("Please call add_route either from Host to Host or NetZone to NetZone") void add_route(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, const std::vector<LinkInRoute>& link_list, bool symmetrical = true);
  /**
   * @brief Add a route between 2 netpoints, and same in other direction
   *
   * Create a route:
   * - route between 2 hosts/routers in same netzone, no gateway is needed
   * - route between 2 netzones, connecting 2 gateways.
   *
   * @param src Source netzone's netpoint
   * @param dst Destination netzone' netpoint
   * @param gw_src Netpoint of the gateway in the source netzone
   * @param gw_dst Netpoint of the gateway in the destination netzone
   * @param link_list List of links
   */
  XBT_ATTRIB_DEPRECATED_v403("Please call add_route either from Host to Host or NetZone to NetZone") void add_route(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, const std::vector<const Link*>& links);
#endif

  /**
   * @brief Add a route between 2 hosts
   *
   * @param src Source host
   * @param dst Destination host
   * @param link_list List of links and their direction used in this communication
   * @param symmetrical Bi-directional communication
   */
  void add_route(const Host* src, const Host* dst, const std::vector<LinkInRoute>& link_list, bool symmetrical = true);
  /**
   * @brief Add a route between 2 hosts
   *
   * @param src Source host
   * @param dst Destination host
   * @param links List of links. The UP direction will be used on src->dst and DOWN direction on dst->src
   */
  void add_route(const Host* src, const Host* dst, const std::vector<const Link*>& links);

  /**
   * @brief Add a route between two netpoints in the same netzone
   *
   * @param src Source netpoint
   * @param dst Destination netpoint
   * @param link_list List of links and their direction used in this communication
   * @param symmetrical Bi-directional communication
   */
  void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                 const std::vector<LinkInRoute>& link_list, bool symmetrical = true);
  /**
   * @brief Add a route between two netpoints in the same netzone
   *
   * @param src Source netpoint
   * @param dst Destination netpoint
   * @param links List of links. The UP direction will be used on src->dst and DOWN direction on dst->src
   */
  void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, const std::vector<const Link*>& links);

  void add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        const std::vector<LinkInRoute>& link_list);

private:
#ifndef DOXYGEN
  static xbt::signal<void(NetZone const&)> on_creation;
  static xbt::signal<void(NetZone const&)> on_seal;
  static xbt::signal<void(NetZone const&)> on_unseal;
#endif

public:
  /** \static Add a callback fired on each newly created NetZone */
  static void on_creation_cb(const std::function<void(NetZone const&)>& cb) { on_creation.connect(cb); }
  /** \static Add a callback fired on each newly sealed NetZone */
  static void on_seal_cb(const std::function<void(NetZone const&)>& cb) { on_seal.connect(cb); }
  /** \static Add a callback fired on each time a NetZone gets unsealed */
  static void on_unseal_cb(const std::function<void(NetZone const&)>& cb) { on_unseal.connect(cb); }

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_host") s4u::Host* create_host(
      const std::string& name, const std::vector<double>& speed_per_pstate)
  {
    return add_host(name, speed_per_pstate);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_host") s4u::Host* create_host(const std::string& name,
                                                                                    double speed)
  {
    return add_host(name, speed);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_host") s4u::Host* create_host(
      const std::string& name, const std::vector<std::string>& speed_per_pstate)
  {
    return add_host(name, speed_per_pstate);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_host") s4u::Host* create_host(const std::string& name,
                                                                                    const std::string& speed)
  {
    return add_host(name, speed);
  }
#endif

  /*** @brief Add a host to a NetZone
   *
   * @param name Host name
   * @param speed_per_pstate Vector of CPU's speeds
   */
  s4u::Host* add_host(const std::string& name, const std::vector<double>& speed_per_pstate);
  s4u::Host* add_host(const std::string& name, double speed);
  /**
   * @brief Add a host to a NetZone (string version)
   *
   * @throw std::invalid_argument if speed format is incorrect.
   */
  s4u::Host* add_host(const std::string& name, const std::vector<std::string>& speed_per_pstate);
  s4u::Host* add_host(const std::string& name, const std::string& speed);

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_link") s4u::Link* create_link(
      const std::string& name, const std::vector<double>& bandwidths)
  {
    return add_link(name, bandwidths);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_link") s4u::Link* create_link(const std::string& name,
                                                                                    double bandwidth)
  {
    return add_link(name, bandwidth);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_link") s4u::Link* create_link(
      const std::string& name, const std::vector<std::string>& bandwidths)
  {
    return add_link(name, bandwidths);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_link") s4u::Link* create_link(const std::string& name,
                                                                                    const std::string& bandwidth)
  {
    return add_link(name, bandwidth);
  }
#endif

  /**
   * @brief Add a link to a NetZone
   *
   * @param name Link name
   * @param bandwidths Link's speed (vector for wifi links)
   * @throw std::invalid_argument if bandwidth format is incorrect.
   */
  s4u::Link* add_link(const std::string& name, const std::vector<double>& bandwidths);
  s4u::Link* add_link(const std::string& name, double bandwidth);

  /** @brief Add a link to a NetZone (string version) */
  s4u::Link* add_link(const std::string& name, const std::vector<std::string>& bandwidths);
  s4u::Link* add_link(const std::string& name, const std::string& bandwidth);

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_split_duplex_link")
      s4u::SplitDuplexLink* create_split_duplex_link(const std::string& name, const std::string& bw_up,
                                                     const std::string& bw_down = "")
  {
    return add_split_duplex_link(name, bw_up, bw_down);
  }
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_split_duplex_link")
      s4u::SplitDuplexLink* create_split_duplex_link(const std::string& name, double bw_up, double bw_down = -1)
  {
    return add_split_duplex_link(name, bw_up, bw_down);
  }
#endif

  /**
   * @brief Create a split-duplex link
   *
   * In SimGrid, split-duplex links are a composition of 2 regular (shared) links (up/down).
   *
   * This function eases its utilization by creating the 2 links for you. We append a suffix
   * "_UP" and "_DOWN" to your link name to identify each of them.
   *
   * Both up/down links have exactly the same bandwidth
   *
   * @param name Name of the link
   * @param bw_up Speed of the up link
   * @param bw_down Speed of the down link (same as up link by default)
   */
  s4u::SplitDuplexLink* add_split_duplex_link(const std::string& name, const std::string& bw_up,
                                              const std::string& bw_down = "");
  s4u::SplitDuplexLink* add_split_duplex_link(const std::string& name, double bw_up, double bw_down = -1);

  kernel::resource::NetworkModel* get_network_model() const;

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_router") kernel::routing::NetPoint* create_router(
      const std::string& name)
  {
    return add_router(name);
  }
#endif
  /**
   * @brief Make a router within that NetZone
   *
   * @param name Router name
   */
  kernel::routing::NetPoint* add_router(const std::string& name);

  /** @brief Seal this netzone configuration */
  NetZone* seal();
  /** @brief Unseal this netzone configuration so that you can modify its content afterward. Do not forget to re-seal()
   * it once you're done */
  NetZone* unseal();

  void
  set_latency_factor_cb(std::function<double(double size, const s4u::Host* src, const s4u::Host* dst,
                                             const std::vector<s4u::Link*>& /*links*/,
                                             const std::unordered_set<s4u::NetZone*>& /*netzones*/)> const& cb) const;
  void
  set_bandwidth_factor_cb(std::function<double(double size, const s4u::Host* src, const s4u::Host* dst,
                                               const std::vector<s4u::Link*>& /*links*/,
                                               const std::unordered_set<s4u::NetZone*>& /*netzones*/)> const& cb) const;

  NetZone* add_netzone_full(const std::string& name);
  NetZone* add_netzone_star(const std::string& name);
  NetZone* add_netzone_dijkstra(const std::string& name, bool cache);
  NetZone* add_netzone_empty(const std::string& name);
  NetZone* add_netzone_floyd(const std::string& name);
  NetZone* add_netzone_vivaldi(const std::string& name);
  NetZone* add_netzone_wifi(const std::string& name);

  NetZone* set_host_cb(const std::function<Host*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb);
  NetZone* set_netzone_cb(const std::function<NetZone*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb);
  NetZone* set_loopback_cb(const std::function<Link*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb);
  NetZone* set_limiter_cb(const std::function<Link*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb);
  /**
   * @brief Add a Fat-Tree zone
   *
   * The best way to understand how  Fat-Tree clusters are characterized is to look at the doc available in:
   * <a href="https://simgrid.org/doc/latest/Platform_examples.html#fat-tree-cluster">Fat Tree Cluster</a>
   *
   * Moreover, this method accepts 3 callbacks to populate the cluster: set_host/set_netzone, set_loopback, and
   * set_limiter .
   *
   * Note that the all elements in a Fat-Tree cluster must have (or not) the same elements (loopback and limiter)
   *
   * @param name NetZone's name
   * @param n_levels  number of levels in the cluster, e.g. 2 (the final tree will have n+1 levels)
   * @param down_links: for each level, how many connections between elements below them, e.g. 2, 3: level 1 nodes are
   * connected to 2 nodes in level 2, which are connected to 3 nodes below. Determines the number total of leaves in the
   * tree.
   * @param up_links for each level, how many connections between elements above themhow nodes, e.g. 1, 2
   * @param link_counts for each level, number of links connecting the nodes, e.g. 1, 1
   * @param bandwidth Characteristics of the inter-nodes link
   * @param latency Characteristics of the inter-nodes link
   * @param sharing_policy Characteristics of the inter-nodes link
   * @return Pointer to new netzone
   */

  NetZone* add_netzone_fatTree(const std::string& name, unsigned int n_levels,
                               const std::vector<unsigned int>& down_links, const std::vector<unsigned int>& up_links,
                               const std::vector<unsigned int>& link_counts, double bandwidth, double latency,
                               Link::SharingPolicy sharing_policy);
  NetZone* add_netzone_fatTree(const std::string& name, unsigned int n_levels,
                               const std::vector<unsigned int>& down_links, const std::vector<unsigned int>& up_links,
                               const std::vector<unsigned int>& links_counts, const std::string& bandwidth,
                               const std::string& latency, Link::SharingPolicy sharing_policy);

  /**
   * @brief Create a torus zone
   *
   * Torus clusters are characterized by:
   * - dimensions, eg. {3,3,3} creates a torus with X = 3 elements, Y = 3 and Z = 3. In total, this cluster have 27
   * elements
   * - inter-node communication: (bandwidth, latency, sharing_policy) the elements are connected through regular links
   * with these characteristics
   * More details in: <a
   * href="https://simgrid.org/doc/latest/Platform_examples.html?highlight=torus#torus-cluster">Torus Cluster</a>
   *
   * Moreover, this method accepts 3 callbacks to populate the cluster: set_netpoint, set_loopback and set_limiter .
   *
   * Note that the all elements in a Torus cluster must have (or not) the same elements (loopback and limiter)
   *
   * @param name NetZone's name
   * @param dimensions List of positive integers (> 0) which determines the torus' dimensions
   * @param bandwidth Characteristics of the inter-nodes link
   * @param latency Characteristics of the inter-nodes link
   * @param sharing_policy Characteristics of the inter-nodes link
   * @return Pointer to new netzone
   */
  NetZone* add_netzone_torus(const std::string& name, const std::vector<unsigned long>& dimensions, double bandwidth,
                             double latency, Link::SharingPolicy sharing_policy);
  NetZone* add_netzone_torus(const std::string& name, const std::vector<unsigned long>& dimensions,
                             const std::string& bandwidth, const std::string& latency,
                             Link::SharingPolicy sharing_policy);

  /**
   * @brief Create a Dragonfly zone
   *
   * In total, Dragonfly clusters will have groups * chassis * routers * nodes elements/leaves.
   *
   * The best way to understand it is looking to the doc available in:
   * <a href="https://simgrid.org/doc/latest/Platform_examples.html#dragonfly-cluster">Dragonfly Cluster</a>
   *
   * Moreover, this method accepts 3 callbacks to populate the cluster: set_netpoint, set_loopback and set_limiter .
   *
   * Note that the all elements in a Dragonfly cluster must have (or not) the same elements (loopback and limiter)
   *
   * @param name NetZone's name
   * @param groups number of groups and links between each group, e.g. 2,2.
   * @param chassis number of chassis in each group and the number of links used to connect the chassis, e.g. 2,3
   * @param routers number of routers in each chassis and their links, e.g. 3,1
   * @param nodes: number of nodes connected to each router using a single link, e.g. 2
   * @param bandwidth Characteristics of the inter-nodes link
   * @param latency Characteristics of the inter-nodes link
   * @param sharing_policy Characteristics of the inter-nodes link
   * @return Pointer to new netzone
   */
  NetZone* add_netzone_dragonfly(const std::string& name, const std::pair<unsigned int, unsigned int>& groups,
                                 const std::pair<unsigned int, unsigned int>& chassis,
                                 const std::pair<unsigned int, unsigned int>& routers,
                                 unsigned int nodes, double bandwidth, double latency, 
                                 Link::SharingPolicy sharing_policy);
  NetZone* add_netzone_dragonfly(const std::string& name, const std::pair<unsigned int, unsigned int>& groups,
                                 const std::pair<unsigned int, unsigned int>& chassis,
                                 const std::pair<unsigned int, unsigned int>& routers,
                                 unsigned int nodes, const std::string& bandwidth, const std::string& latency, 
                                 Link::SharingPolicy sharing_policy);
};

#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_full or NetZone::add_netzone_full")
    XBT_PUBLIC NetZone* create_full_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_star or NetZone::add_netzone_star")
    XBT_PUBLIC NetZone* create_star_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_dijkstra or NetZone::add_netzone_dijkstra")
    XBT_PUBLIC NetZone* create_dijkstra_zone(const std::string& name, bool cache);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_empty or NetZone::add_netzone_empty")
    XBT_PUBLIC NetZone* create_empty_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_floyd or NetZone::add_netzone_floyd")
    XBT_PUBLIC NetZone* create_floyd_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_vivaldi or NetZone::add_netzone_vivaldi")
    XBT_PUBLIC NetZone* create_vivaldi_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use Engine::set_rootnetzone_wifi or NetZone::add_netzone_wifi")
    XBT_PUBLIC NetZone* create_wifi_zone(const std::string& name);
XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_netzone_fatTree") XBT_PUBLIC NetZone* create_fatTree_zone(
    const std::string& name, NetZone* parent, const FatTreeParams& parameters, const ClusterCallbacks& set_callbacks,
    double bandwidth, double latency, Link::SharingPolicy sharing_policy);
XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_netzone_fatTree") XBT_PUBLIC NetZone* create_torus_zone(
    const std::string& name, NetZone* parent, const std::vector<unsigned long>& dimensions,
    const ClusterCallbacks& set_callbacks, double bandwidth, double latency, Link::SharingPolicy sharing_policy);
XBT_ATTRIB_DEPRECATED_v403("Please use NetZone::add_netzone_dragonfly")
XBT_PUBLIC NetZone* create_dragonfly_zone(const std::string& name, NetZone* parent,
    const DragonflyParams& parameters, const ClusterCallbacks& set_callbacks,
    double bandwidth, double latency, Link::SharingPolicy sharing_policy);
#endif

} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_NETZONE_HPP */
