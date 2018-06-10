/* Copyright (c) 2016-2018. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_NETZONE_HPP
#define SIMGRID_S4U_NETZONE_HPP

#include <simgrid/forward.h>
#include <xbt/signal.hpp>

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
protected:
  friend simgrid::kernel::routing::NetZoneImpl;

  explicit NetZone(NetZone * father, std::string name);
  virtual ~NetZone();

public:
  /** @brief Seal your netzone once you're done adding content, and before routing stuff through it */
  virtual void seal();
  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& get_name() const { return name_; }
  /** @brief Retrieves the name of that netzone as a C string */
  const char* get_cname() const;
  NetZone* get_father();

  std::vector<Host*> get_all_hosts();

  std::vector<NetZone*>* getChildren();             // Sub netzones
  int getHostCount();

private:
  std::unordered_map<std::string, std::string> properties_;

public:
  /** Get the properties assigned to a netzone */
  std::unordered_map<std::string, std::string>* get_properties();

  /** Retrieve the property value (or nullptr if not set) */
  const char* get_property(const char* key);
  void set_property(const char* key, const char* value);

  /* Add content to the netzone, at parsing time. It should be sealed afterward. */
  virtual int add_component(kernel::routing::NetPoint* elm); /* A host, a router or a netzone, whatever */
  virtual void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                         kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                         std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical);
  virtual void add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                                kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                                std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical) = 0;

  /*** Called on each newly created regular route (not on bypass routes) */
  static simgrid::xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                                   kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                                   std::vector<kernel::resource::LinkImpl*>& link_list)>
      on_route_creation;
  static simgrid::xbt::signal<void(NetZone&)> on_creation;
  static simgrid::xbt::signal<void(NetZone&)> on_seal;

private:
  // our content, as known to our graph routing algorithm (maps vertexId -> vertex)
  std::vector<kernel::routing::NetPoint*> vertices_;

protected:
  unsigned int get_table_size() { return vertices_.size(); }
  std::vector<kernel::routing::NetPoint*> get_vertices() { return vertices_; }

private:
  NetZone* father_ = nullptr;
  std::string name_;

  bool sealed_ = false; // We cannot add more content when sealed

  std::vector<NetZone*>* children_ = nullptr; // sub-netzones

public: // Deprecation wrappers
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_father()") NetZone* getFather() { return get_father(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_name()") const std::string& getName() const { return get_name(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_cname()") const char* getCname() const { return get_cname(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_route()") void addRoute(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, std::vector<simgrid::kernel::resource::LinkImpl*>& link_list, bool symmetrical)
  {
    add_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_bypass_route()") void addBypassRoute(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, std::vector<simgrid::kernel::resource::LinkImpl*>& link_list, bool symmetrical)
  {
    add_bypass_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
  }
  XBT_ATTRIB_DEPRECATED_v323(
      "Please use NetZone::get_properties()") std::unordered_map<std::string, std::string>* getProperties()
  {
    return get_properties();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_property()") const char* getProperty(const char* key)
  {
    return get_property(key);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::set_property()") void setProperty(const char* key, const char* value)
  {
    set_property(key, value);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_component()") virtual int addComponent(
      kernel::routing::NetPoint* elm)
  {
    return add_component(elm);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_vertices()") std::vector<kernel::routing::NetPoint*> getVertices()
  {
    return get_vertices();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_all_hosts()") void getHosts(
      std::vector<s4u::Host*>* whereto); // retrieve my content as a vector of hosts
};
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_NETZONE_HPP */
