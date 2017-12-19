/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_NETZONE_HPP
#define SIMGRID_S4U_NETZONE_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <xbt/base.h>
#include <xbt/graph.h>
#include <xbt/signal.hpp>

#include <simgrid/forward.h>
#include <simgrid/s4u/forward.hpp>

namespace simgrid {

namespace s4u {

/** @brief Networking Zones
 *
 * A netzone is a network container, in charge of routing information between elements (hosts) and to the nearby
 * netzones. In SimGrid, there is a hierarchy of netzones, with a unique root zone (that you can retrieve from the
 * s4u::Engine).
 */
XBT_PUBLIC_CLASS NetZone
{
protected:
  friend simgrid::kernel::routing::NetZoneImpl;

  explicit NetZone(NetZone * father, std::string name);
  virtual ~NetZone();

public:
  /** @brief Seal your netzone once you're done adding content, and before routing stuff through it */
  virtual void seal();
  /** @brief Retrieves the name of that netzone as a C++ string */
  const std::string& getName() const { return name_; }
  /** @brief Retrieves the name of that netzone as a C string */
  const char* getCname() const;
  NetZone* getFather();

  std::vector<NetZone*>* getChildren();             // Sub netzones
  void getHosts(std::vector<s4u::Host*> * whereto); // retrieve my content as a vector of hosts

  /** Get the properties assigned to a host */
  std::unordered_map<std::string, std::string>* getProperties();

  /** Retrieve the property value (or nullptr if not set) */
  const char* getProperty(const char* key);
  void setProperty(const char* key, const char* value);

  /* Add content to the netzone, at parsing time. It should be sealed afterward. */
  virtual int addComponent(kernel::routing::NetPoint * elm); /* A host, a router or a netzone, whatever */
  virtual void addRoute(kernel::routing::NetPoint * src, kernel::routing::NetPoint * dst,
                        kernel::routing::NetPoint * gw_src, kernel::routing::NetPoint * gw_dst,
                        std::vector<simgrid::surf::LinkImpl*> & link_list, bool symmetrical);
  virtual void addBypassRoute(kernel::routing::NetPoint * src, kernel::routing::NetPoint * dst,
                              kernel::routing::NetPoint * gw_src, kernel::routing::NetPoint * gw_dst,
                              std::vector<simgrid::surf::LinkImpl*> & link_list, bool symmetrical) = 0;

  /*** Called on each newly created regular route (not on bypass routes) */
  static simgrid::xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                                   kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                                   std::vector<surf::LinkImpl*>& link_list)>
      onRouteCreation;
  static simgrid::xbt::signal<void(NetZone&)> onCreation;
  static simgrid::xbt::signal<void(NetZone&)> onSeal;

protected:
  unsigned int getTableSize() { return vertices_.size(); }
  std::vector<kernel::routing::NetPoint*> getVertices() { return vertices_; }

private:
  // our content, as known to our graph routing algorithm (maps vertexId -> vertex)
  std::vector<kernel::routing::NetPoint*> vertices_;

  std::unordered_map<std::string, std::string> properties_;
  NetZone* father_ = nullptr;
  std::string name_;

  bool sealed_ = false; // We cannot add more content when sealed

  std::vector<NetZone*>* children_ = nullptr; // sub-netzones
};
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_NETZONE_HPP */
