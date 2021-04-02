/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_NETZONE_HPP
#define SIMGRID_S4U_NETZONE_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Link.hpp>
#include <xbt/graph.h>
#include <xbt/signal.hpp>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simgrid {
namespace s4u {

/** @brief Networking Zones
 *
 * A netzone is a network container, in charge of routing information between elements (hosts) and to the nearby
 * netzones. In SimGrid, there is a hierarchy of netzones, with a unique root zone (that you can retrieve from the
 * s4u::Engine).
 */
class XBT_PUBLIC NetZone {
  kernel::routing::NetZoneImpl* const pimpl_;

protected:
  friend kernel::routing::NetZoneImpl;

  explicit NetZone(kernel::routing::NetZoneImpl* impl) : pimpl_(impl) {}

public:
  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that netzone as a C string */
  const char* get_cname() const;

  XBT_ATTRIB_DEPRECATED_v331("Please use get_parent()") NetZone* get_father();
  NetZone* get_parent() const;
  NetZone* set_parent(const NetZone* parent);

  std::vector<Host*> get_all_hosts() const;
  int get_host_count() const;

  kernel::routing::NetZoneImpl* get_impl() const { return pimpl_; }

  /** Get the properties assigned to a netzone */
  const std::unordered_map<std::string, std::string>* get_properties() const;
  /** Retrieve the property value (or nullptr if not set) */
  const char* get_property(const std::string& key) const;
  void set_property(const std::string& key, const std::string& value);

  std::vector<NetZone*> get_children() const;
  NetZone* add_child(const NetZone* new_zone);

  void extract_xbt_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                         std::map<std::string, xbt_edge_t, std::less<>>* edges);

  /* Add content to the netzone, at parsing time. It should be sealed afterward. */
  int add_component(kernel::routing::NetPoint* elm); /* A host, a router or a netzone, whatever */
  void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
                 kernel::routing::NetPoint* gw_dst, std::vector<kernel::resource::LinkImpl*>& link_list,
                 bool symmetrical);
  void add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical);

  /*** Called on each newly created regular route (not on bypass routes) */
  static xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                          kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                          std::vector<kernel::resource::LinkImpl*> const& link_list)>
      on_route_creation;
  static xbt::signal<void(NetZone const&)> on_creation;
  static xbt::signal<void(NetZone const&)> on_seal;

  /**
   * @brief Create a host
   *
   * @param name Host name
   * @param speed_per_state Vector of CPU's speeds
   */
  s4u::Host* create_host(const std::string& name, const std::vector<double>& speed_per_pstate);
  /**
   * @brief Create a Host (string version)
   *
   * @throw std::invalid_argument if speed format is incorrect.
   */
  s4u::Host* create_host(const std::string& name, const std::vector<std::string>& speed_per_pstate);

  /**
   * @brief Create a link
   *
   * @param name Link name
   * @param bandwidths Link's speed (vector for wifi links)
   * @param policy Link sharing policy
   * @throw std::invalid_argument if bandwidth format is incorrect.
   */
  s4u::Link* create_link(const std::string& name, const std::vector<double>& bandwidths,
                         Link::SharingPolicy policy = Link::SharingPolicy::SHARED);

  /** @brief Create a link (string version) */
  s4u::Link* create_link(const std::string& name, const std::vector<std::string>& bandwidths,
                         Link::SharingPolicy policy = Link::SharingPolicy::SHARED);
};

// External constructors so that the types (and the types of their content) remain hidden
XBT_PUBLIC NetZone* create_full_zone(const std::string& name);
XBT_PUBLIC NetZone* create_cluster_zone(const std::string& name);
XBT_PUBLIC NetZone* create_dijkstra_zone(const std::string& name, bool cache);
XBT_PUBLIC NetZone* create_dragonfly_zone(const std::string& name);
XBT_PUBLIC NetZone* create_empty_zone(const std::string& name);
XBT_PUBLIC NetZone* create_fatTree_zone(const std::string& name);
XBT_PUBLIC NetZone* create_floyd_zone(const std::string& name);
XBT_PUBLIC NetZone* create_torus_zone(const std::string& name);
XBT_PUBLIC NetZone* create_vivaldi_zone(const std::string& name);
XBT_PUBLIC NetZone* create_wifi_zone(const std::string& name);

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_NETZONE_HPP */
