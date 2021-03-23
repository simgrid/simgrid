/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/simix.hpp"
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
  return pimpl_->get_properties();
}

/** Retrieve the property value (or nullptr if not set) */
const char* NetZone::get_property(const std::string& key) const
{
  return pimpl_->get_property(key);
}

void NetZone::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { pimpl_->set_property(key, value); });
}

/** @brief Returns the list of direct children (no grand-children) */
std::vector<NetZone*> NetZone::get_children() const
{
  std::vector<NetZone*> res;
  for (auto child : *(pimpl_->get_children()))
    res.push_back(child->get_iface());
  return res;
}

NetZone* NetZone::add_child(const NetZone* new_zone)
{
  pimpl_->add_child(new_zone->get_impl());
  return this;
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
  return pimpl_->get_parent()->get_iface();
}

NetZone* NetZone::get_parent() const
{
  return pimpl_->get_parent()->get_iface();
}

NetZone* NetZone::set_parent(const NetZone* parent)
{
  pimpl_->set_parent(parent->get_impl());
  return this;
}

/** @brief Returns the list of the hosts found in this NetZone (not recursively)
 *
 * Only the hosts that are directly contained in this NetZone are retrieved,
 * not the ones contained in sub-netzones.
 */
std::vector<Host*> NetZone::get_all_hosts() const
{
  return pimpl_->get_all_hosts();
}

int NetZone::get_host_count() const
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

void NetZone::extract_xbt_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                                std::map<std::string, xbt_edge_t, std::less<>>* edges)
{
  for (auto const& child : get_children())
    child->extract_xbt_graph(graph, nodes, edges);

  pimpl_->get_graph(graph, nodes, edges);
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

sg_netzone_t sg_zone_get_root()
{
  return simgrid::s4u::Engine::get_instance()->get_netzone_root();
}

const char* sg_zone_get_name(const_sg_netzone_t netzone)
{
  return netzone->get_cname();
}

sg_netzone_t sg_zone_get_by_name(const char* name)
{
  return simgrid::s4u::Engine::get_instance()->netzone_by_name_or_null(name);
}

void sg_zone_get_sons(const_sg_netzone_t netzone, xbt_dict_t whereto)
{
  for (auto const& elem : netzone->get_children()) {
    xbt_dict_set(whereto, elem->get_cname(), elem);
  }
}

const char* sg_zone_get_property_value(const_sg_netzone_t netzone, const char* name)
{
  return netzone->get_property(name);
}

void sg_zone_set_property_value(sg_netzone_t netzone, const char* name, const char* value)
{
  netzone->set_property(name, value);
}

void sg_zone_get_hosts(const_sg_netzone_t netzone, xbt_dynar_t whereto)
{
  /* converts vector to dynar */
  std::vector<simgrid::s4u::Host*> hosts = netzone->get_all_hosts();
  for (auto const& host : hosts)
    xbt_dynar_push(whereto, &host);
}
