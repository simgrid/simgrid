/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/simix.hpp"
#include "src/surf/network_interface.hpp" // Link FIXME: move to proper header
#include "xbt/log.h"
#include <simgrid/zone.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_netzone, "S4U Networking Zones");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                          kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                          std::vector<surf::LinkImpl*>& link_list)>
    NetZone::onRouteCreation;
simgrid::xbt::signal<void(NetZone&)> NetZone::onCreation;
simgrid::xbt::signal<void(NetZone&)> NetZone::onSeal;

NetZone::NetZone(NetZone* father, std::string name) : father_(father), name_(name)
{
  children_ = new std::vector<NetZone*>();
}

void NetZone::seal()
{
  sealed_ = true;
}

NetZone::~NetZone()
{
  for (auto const& nz : *children_)
    delete nz;
  delete children_;
}

std::unordered_map<std::string, std::string>* NetZone::getProperties()
{
  return simgrid::simix::kernelImmediate([this] {
      return &properties_;
  });
}

/** Retrieve the property value (or nullptr if not set) */
const char* NetZone::getProperty(const char* key)
{
  return properties_.at(key).c_str();
}
void NetZone::setProperty(const char* key, const char* value)
{
  simgrid::simix::kernelImmediate([this,key,value] {
    properties_[key] = value;
  });
}

/** @brief Returns the list of direct children (no grand-children)
 *
 * This function returns the internal copy of the children, not a copy. Don't mess with it!
 */
std::vector<NetZone*>* NetZone::getChildren()
{
  return children_;
}

const char* NetZone::getCname() const
{
  return name_.c_str();
}
NetZone* NetZone::getFather()
{
  return father_;
}

void NetZone::getHosts(std::vector<s4u::Host*>* whereto)
{
  for (auto const& card : vertices_) {
    s4u::Host* host = simgrid::s4u::Host::by_name_or_null(card->getName());
    if (host != nullptr)
      whereto->push_back(host);
  }
}

int NetZone::getHostCount()
{
  int count = 0;
  for (auto const& card : vertices_) {
    s4u::Host* host = simgrid::s4u::Host::by_name_or_null(card->getName());
    if (host != nullptr)
      count++;
  }
  return count;
}

int NetZone::addComponent(kernel::routing::NetPoint* elm)
{
  vertices_.push_back(elm);
  return vertices_.size() - 1; // The rank of the newly created object
}

void NetZone::addRoute(sg_netpoint_t /*src*/, sg_netpoint_t /*dst*/, sg_netpoint_t /*gw_src*/, sg_netpoint_t /*gw_dst*/,
                       std::vector<simgrid::surf::LinkImpl*>& /*link_list*/, bool /*symmetrical*/)
{
  xbt_die("NetZone '%s' does not accept new routes (wrong class).", name_.c_str());
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
SG_BEGIN_DECL()
sg_netzone_t sg_zone_get_root()
{
  return simgrid::s4u::Engine::getInstance()->getNetRoot();
}

const char* sg_zone_get_name(sg_netzone_t netzone)
{
  return netzone->getCname();
}

sg_netzone_t sg_zone_get_by_name(const char* name)
{
  return simgrid::s4u::Engine::getInstance()->getNetzoneByNameOrNull(name);
}

void sg_zone_get_sons(sg_netzone_t netzone, xbt_dict_t whereto)
{
  for (auto const& elem : *netzone->getChildren()) {
    xbt_dict_set(whereto, elem->getCname(), static_cast<void*>(elem), nullptr);
  }
}

const char* sg_zone_get_property_value(sg_netzone_t netzone, const char* name)
{
  return netzone->getProperty(name);
}

void sg_zone_set_property_value(sg_netzone_t netzone, const char* name, char* value)
{
  netzone->setProperty(name, value);
}

void sg_zone_get_hosts(sg_netzone_t netzone, xbt_dynar_t whereto)
{
  /* converts vector to dynar */
  std::vector<simgrid::s4u::Host*> hosts;
  netzone->getHosts(&hosts);
  for (auto const& host : hosts)
    xbt_dynar_push(whereto, &host);
}

SG_END_DECL()
