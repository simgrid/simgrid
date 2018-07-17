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

  explicit NetZone(kernel::routing::NetZoneImpl* impl);
  ~NetZone();

public:
  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that netzone as a C string */
  const char* get_cname() const;

  NetZone* get_father();

  std::vector<Host*> get_all_hosts();
  int get_host_count();

  kernel::routing::NetZoneImpl* get_impl() { return pimpl_; }

private:
  kernel::routing::NetZoneImpl* pimpl_;
  std::unordered_map<std::string, std::string> properties_;

public:
  /** Get the properties assigned to a netzone */
  std::unordered_map<std::string, std::string>* get_properties();

  std::vector<NetZone*> get_children();

  /** Retrieve the property value (or nullptr if not set) */
  const char* get_property(std::string key);
  void set_property(std::string key, std::string value);

  /* Add content to the netzone, at parsing time. It should be sealed afterward. */
  int add_component(kernel::routing::NetPoint* elm); /* A host, a router or a netzone, whatever */
  void add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
                 kernel::routing::NetPoint* gw_dst, std::vector<kernel::resource::LinkImpl*>& link_list,
                 bool symmetrical);
  void add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical);

  /*** Called on each newly created regular route (not on bypass routes) */
  static simgrid::xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                                   kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                                   std::vector<kernel::resource::LinkImpl*>& link_list)>
      on_route_creation;
  static simgrid::xbt::signal<void(NetZone&)> on_creation;
  static simgrid::xbt::signal<void(NetZone&)> on_seal;

  // Deprecation wrappers
  /** @deprecated NetZone::get_father() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_father()") NetZone* getFather() { return get_father(); }
  /** @deprecated NetZone::get_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_name()") const std::string& getName() const { return get_name(); }
  /** @deprecated NetZone::get_cname() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_cname()") const char* getCname() const { return get_cname(); }
  /** @deprecated NetZone::add_route() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_route()") void addRoute(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, std::vector<simgrid::kernel::resource::LinkImpl*>& link_list, bool symmetrical)
  {
    add_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
  }
  /** @deprecated NetZone::add_bypass_route() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_bypass_route()") void addBypassRoute(
      kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
      kernel::routing::NetPoint* gw_dst, std::vector<simgrid::kernel::resource::LinkImpl*>& link_list, bool symmetrical)
  {
    add_bypass_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
  }
  /** @deprecated NetZone::get_properties() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_properties()") std::unordered_map<std::string, std::string>* getProperties()
  {
    return get_properties();
  }
  /** @deprecated NetZone::get_property() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_property()") const char* getProperty(const char* key)
  {
    return get_property(key);
  }
  /** @deprecated NetZone::set_property() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::set_property()") void setProperty(const char* key, const char* value)
  {
    set_property(key, value);
  }
  /** @deprecated NetZone::add_component() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::add_component()") int addComponent(kernel::routing::NetPoint* elm)
  {
    return add_component(elm);
  }
  /** @deprecated NetZone::get_vertices() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_vertices()") std::vector<kernel::routing::NetPoint*> getVertices();
  /** @deprecated NetZone::get_host_count() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_host_count()") int getHostCount() { return get_host_count(); }
  /** @deprecated NetZone::get_all_hosts() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_all_hosts()") void getHosts(
      std::vector<s4u::Host*>* whereto); // retrieve my content as a vector of hosts
  /** @deprecated NetZone::get_children() */
  XBT_ATTRIB_DEPRECATED_v323("Please use NetZone::get_children()") std::vector<NetZone*>* getChildren()
  {
    std::vector<NetZone*>* res = new std::vector<NetZone*>();
    for (auto child : get_children())
      res->push_back(child);
    return res;
  }
};
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_NETZONE_HPP */
