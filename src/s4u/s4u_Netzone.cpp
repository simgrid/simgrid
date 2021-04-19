/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/simix.hpp"
#include "simgrid/zone.h"
#include "xbt/parse_units.hpp"

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
  for (auto child : pimpl_->get_children())
    res.push_back(child->get_iface());
  return res;
}

NetZone* NetZone::add_child(NetZone* new_zone)
{
  new_zone->set_parent(this);
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
  kernel::actor::simcall([this, parent] { pimpl_->set_parent(parent->get_impl()); });
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

std::vector<kernel::resource::LinkImpl*> NetZone::get_link_list_impl(const std::vector<Link*> link_list)
{
  std::vector<kernel::resource::LinkImpl*> links;
  for (const auto& link : link_list) {
    links.push_back(link->get_impl());
  }
  return links;
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        const std::vector<Link*>& link_list, bool symmetrical)
{
  pimpl_->add_route(src, dst, gw_src, gw_dst, NetZone::get_link_list_impl(link_list), symmetrical);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        const std::vector<kernel::resource::LinkImpl*>& link_list, bool symmetrical)
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

NetZone* NetZone::seal()
{
  kernel::actor::simcall([this] { pimpl_->seal(); });
  return this;
}

s4u::Host* NetZone::create_host(const std::string& name, double speed)
{
  return create_host(name, std::vector<double>{speed});
}

s4u::Host* NetZone::create_host(const std::string& name, const std::vector<double>& speed_per_pstate)
{
  return kernel::actor::simcall(
      [this, &name, &speed_per_pstate] { return pimpl_->create_host(name, speed_per_pstate); });
}

s4u::Host* NetZone::create_host(const std::string& name, const std::string& speed)
{
  return create_host(name, std::vector<std::string>{speed});
}

s4u::Host* NetZone::create_host(const std::string& name, const std::vector<std::string>& speed_per_pstate)
{
  return create_host(name, Host::convert_pstate_speed_vector(speed_per_pstate));
}

s4u::Link* NetZone::create_link(const std::string& name, double bandwidth)
{
  return create_link(name, std::vector<double>{bandwidth});
}

s4u::Link* NetZone::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  return kernel::actor::simcall([this, &name, &bandwidths] { return pimpl_->create_link(name, bandwidths); });
}

s4u::Link* NetZone::create_link(const std::string& name, const std::string& bandwidth)
{
  return create_link(name, std::vector<std::string>{bandwidth});
}

s4u::Link* NetZone::create_link(const std::string& name, const std::vector<std::string>& bandwidths)
{
  std::vector<double> bw;
  bw.reserve(bandwidths.size());
  for (const auto& speed_str : bandwidths) {
    try {
      double speed = xbt_parse_get_bandwidth("", 0, speed_str.c_str(), nullptr, "");
      bw.push_back(speed);
    } catch (const simgrid::ParseError&) {
      throw std::invalid_argument(std::string("Impossible to create link: ") + name +
                                  std::string(". Invalid bandwidth: ") + speed_str);
    }
  }
  return create_link(name, bw);
}

kernel::routing::NetPoint* NetZone::create_router(const std::string& name)
{
  return kernel::actor::simcall([this, &name] { return pimpl_->create_router(name); });
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
