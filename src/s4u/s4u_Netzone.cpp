/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/zone.h"

namespace simgrid {
namespace s4u {

xbt::signal<void(bool symmetrical, kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                 kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                 std::vector<kernel::resource::LinkImpl*> const& link_list)>
    NetZone::on_route_creation;
xbt::signal<void(NetZone const&)> NetZone::on_creation;
xbt::signal<void(NetZone const&)> NetZone::on_seal;

const std::unordered_map<std::string, std::string>* NetZone::get_properties() const
{
  return &properties_;
}

/** Retrieve the property value (or nullptr if not set) */
const char* NetZone::get_property(const std::string& key) const
{
  auto prop = properties_.find(key);
  return prop == properties_.end() ? nullptr : prop->second.c_str();
}

void NetZone::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { properties_[key] = value; });
}

/** @brief Returns the list of direct children (no grand-children) */
std::vector<NetZone*> NetZone::get_children()
{
  std::vector<NetZone*> res;
  for (auto child : *(pimpl_->get_children()))
    res.push_back(child->get_iface());
  return res;
}

const std::string& NetZone::get_name() const
{
  return pimpl_->get_name();
}
const char* NetZone::get_cname() const
{
  return pimpl_->get_cname();
}
NetZone* NetZone::get_father()
{
  return pimpl_->get_father()->get_iface();
}

/** @brief Returns the list of the hosts found in this NetZone (not recursively)
 *
 * Only the hosts that are directly contained in this NetZone are retrieved,
 * not the ones contained in sub-netzones.
 */
std::vector<Host*> NetZone::get_all_hosts()
{
  return pimpl_->get_all_hosts();
}

int NetZone::get_host_count()
{
  return pimpl_->get_host_count();
}

int NetZone::add_component(kernel::routing::NetPoint* elm)
{
  return pimpl_->add_component(elm);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical)
{
  pimpl_->add_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
}
void NetZone::add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                               kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                               std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical)
{
  pimpl_->add_bypass_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

sg_netzone_t sg_zone_get_root()
{
  return simgrid::s4u::Engine::get_instance()->get_netzone_root();
}

const char* sg_zone_get_name(sg_netzone_t netzone)
{
  return netzone->get_cname();
}

sg_netzone_t sg_zone_get_by_name(const char* name)
{
  return simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null(name);
}

void sg_zone_get_sons(sg_netzone_t netzone, xbt_dict_t whereto)
{
  for (auto const& elem : netzone->get_children()) {
    xbt_dict_set(whereto, elem->get_cname(), static_cast<void*>(elem));
  }
}

const char* sg_zone_get_property_value(sg_netzone_t netzone, const char* name)
{
  return netzone->get_property(name);
}

void sg_zone_set_property_value(sg_netzone_t netzone, const char* name, char* value)
{
  netzone->set_property(name, value);
}

void sg_zone_get_hosts(sg_netzone_t netzone, xbt_dynar_t whereto)
{
  /* converts vector to dynar */
  std::vector<simgrid::s4u::Host*> hosts = netzone->get_all_hosts();
  for (auto const& host : hosts)
    xbt_dynar_push(whereto, &host);
}
