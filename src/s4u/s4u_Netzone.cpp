/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/DijkstraZone.hpp"
#include "simgrid/kernel/routing/EmptyZone.hpp"
#include "simgrid/kernel/routing/FloydZone.hpp"
#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/StarZone.hpp"
#include "simgrid/kernel/routing/VivaldiZone.hpp"
#include "simgrid/kernel/routing/WifiZone.hpp"
#include "simgrid/s4u/Host.hpp"
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/simcall.hpp>
#include <simgrid/zone.h>
#include <xbt/asserts.hpp>
#include <xbt/parse_units.hpp>

#include "src/kernel/resource/NetworkModel.hpp"

namespace simgrid::s4u {

xbt::signal<void(NetZone const&)> NetZone::on_creation;
xbt::signal<void(NetZone const&)> NetZone::on_seal;
xbt::signal<void(NetZone const&)> NetZone::on_unseal;

const std::unordered_map<std::string, std::string>* NetZone::get_properties() const
{
  return pimpl_->get_properties();
}

NetZone* NetZone::set_properties(const std::unordered_map<std::string, std::string>& properties)
{
  pimpl_->set_properties(properties);
  return this;
}

/** Retrieve the property value (or nullptr if not set) */
const char* NetZone::get_property(const std::string& key) const
{
  return pimpl_->get_property(key);
}

void NetZone::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall_answered([this, &key, &value] { pimpl_->set_property(key, value); });
}

/** @brief Returns the list of direct children (no grand-children) */
std::vector<NetZone*> NetZone::get_children() const
{
  std::vector<NetZone*> res;
  for (auto* child : pimpl_->get_children())
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

NetZone* NetZone::get_parent() const
{
  return pimpl_->get_parent()->get_iface();
}

NetZone* NetZone::set_parent(const NetZone* parent)
{
  kernel::actor::simcall_answered([this, parent] { pimpl_->set_parent(parent->get_impl()); });
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

size_t NetZone::get_host_count() const
{
  return pimpl_->get_host_count();
}

unsigned long NetZone::add_component(kernel::routing::NetPoint* elm)
{
  return pimpl_->add_component(elm);
}

void NetZone::add_route(const NetZone* src, const NetZone* dst, const std::vector<const Link*>& links)
{
  std::vector<LinkInRoute> links_direct;
  std::vector<LinkInRoute> links_reverse;
  for (auto* l : links) {
    links_direct.emplace_back(LinkInRoute(l, LinkInRoute::Direction::UP));
    links_reverse.emplace_back(LinkInRoute(l, LinkInRoute::Direction::DOWN));
  }
  pimpl_->add_route(src ? src->get_netpoint() : nullptr, dst ? dst->get_netpoint(): nullptr,
                    src ? src->get_gateway() : nullptr, dst ? dst->get_gateway() : nullptr,
                    links_direct, false);
  pimpl_->add_route(dst ? dst->get_netpoint(): nullptr, src ? src->get_netpoint() : nullptr,
                    dst ? dst->get_gateway() : nullptr, src ? src->get_gateway() : nullptr,
                    links_reverse, false);
}

void NetZone::add_route(const NetZone* src, const NetZone* dst, const std::vector<LinkInRoute>& link_list, bool symmetrical)
{
  pimpl_->add_route(src ? src->get_netpoint() : nullptr, dst ? dst->get_netpoint(): nullptr,
                    src ? src->get_gateway() : nullptr, dst ? dst->get_gateway() : nullptr,
                    link_list, symmetrical);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        const std::vector<LinkInRoute>& link_list, bool symmetrical) // XBT_ATTRIB_DEPRECATED_v401
{
  pimpl_->add_route(src, dst, gw_src, gw_dst, link_list, symmetrical);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                        kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                        const std::vector<const Link*>& links) // XBT_ATTRIB_DEPRECATED_v401
{
  std::vector<LinkInRoute> links_direct;
  std::vector<LinkInRoute> links_reverse;
  for (auto* l : links) {
    links_direct.emplace_back(LinkInRoute(l, LinkInRoute::Direction::UP));
    links_reverse.emplace_back(LinkInRoute(l, LinkInRoute::Direction::DOWN));
  }
  pimpl_->add_route(src, dst, gw_src, gw_dst, links_direct, false);
  pimpl_->add_route(dst, src, gw_dst, gw_src, links_reverse, false);
}

void NetZone::add_route(const Host* src, const Host* dst, const std::vector<LinkInRoute>& link_list, bool symmetrical)
{
  pimpl_->add_route(src ? src->get_netpoint(): nullptr, dst ? dst->get_netpoint(): nullptr, nullptr, nullptr,
                    link_list, symmetrical);

}

void NetZone::add_route(const Host* src, const Host* dst, const std::vector<const Link*>& links)
{
  std::vector<LinkInRoute> links_direct;
  std::vector<LinkInRoute> links_reverse;
  for (auto* l : links) {
    links_direct.emplace_back(LinkInRoute(l, LinkInRoute::Direction::UP));
    links_reverse.emplace_back(LinkInRoute(l, LinkInRoute::Direction::DOWN));
  }
  pimpl_->add_route(src ? src->get_netpoint(): nullptr, dst ? dst->get_netpoint(): nullptr, nullptr, nullptr,
                    links_direct, false);
  pimpl_->add_route(dst ? dst->get_netpoint(): nullptr, src ? src->get_netpoint(): nullptr, nullptr, nullptr,
                    links_reverse, false);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, const std::vector<LinkInRoute>& link_list, bool symmetrical)
{
  xbt_assert(src->get_englobing_zone() == dst->get_englobing_zone(), "Netpoints have to be in the same NetZone");
  pimpl_->add_route(src, dst, nullptr, nullptr, link_list, symmetrical);
}

void NetZone::add_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, const std::vector<const Link*>& links)
{
  xbt_assert(src->get_englobing_zone() == dst->get_englobing_zone(), "Netpoints have to be in the same NetZone");
  std::vector<LinkInRoute> links_direct;
  std::vector<LinkInRoute> links_reverse;
  for (auto* l : links) {
    links_direct.emplace_back(LinkInRoute(l, LinkInRoute::Direction::UP));
    links_reverse.emplace_back(LinkInRoute(l, LinkInRoute::Direction::DOWN));
  }
  pimpl_->add_route(src, dst, nullptr, nullptr, links_direct, false);
  pimpl_->add_route(dst, src, nullptr, nullptr, links_reverse, false);
}

void NetZone::add_bypass_route(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst,
                               kernel::routing::NetPoint* gw_src, kernel::routing::NetPoint* gw_dst,
                               const std::vector<LinkInRoute>& link_list)
{
  pimpl_->add_bypass_route(src, dst, gw_src, gw_dst, link_list);
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
  kernel::actor::simcall_answered([this] { pimpl_->seal(); });
  return this;
}
NetZone* NetZone::unseal()
{
  kernel::actor::simcall_answered([this] { pimpl_->unseal(); });
  return this;
}
void NetZone::set_latency_factor_cb(
    std::function<double(double size, const s4u::Host* src, const s4u::Host* dst,
                         const std::vector<s4u::Link*>& /*links*/,
                         const std::unordered_set<s4u::NetZone*>& /*netzones*/)> const& cb) const
{
  kernel::actor::simcall_answered([this, &cb]() { pimpl_->get_network_model()->set_lat_factor_cb(cb); });
}
void NetZone::set_bandwidth_factor_cb(
    std::function<double(double size, const s4u::Host* src, const s4u::Host* dst,
                         const std::vector<s4u::Link*>& /*links*/,
                         const std::unordered_set<s4u::NetZone*>& /*netzones*/)> const& cb) const
{
  kernel::actor::simcall_answered([this, &cb]() { pimpl_->get_network_model()->set_bw_factor_cb(cb); });
}

NetZone* NetZone::set_host_cb(const std::function<Host*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb)
{
  xbt_assert(dynamic_cast<kernel::routing::ClusterBase*>(get_impl()) != nullptr, 
             "set_loopback_cb can only be called for ClusterZones");
  kernel::actor::simcall_answered([this, &cb]() { static_cast<kernel::routing::ClusterBase*>(pimpl_)->set_host_cb(cb); });           
  return this;
}

NetZone* NetZone::set_netzone_cb(const std::function<NetZone*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb)
{
  xbt_assert(dynamic_cast<kernel::routing::ClusterBase*>(get_impl()) != nullptr, 
             "set_loopback_cb can only be called for ClusterZones");
  kernel::actor::simcall_answered([this, &cb]() { static_cast<kernel::routing::ClusterBase*>(pimpl_)->set_netzone_cb(cb); });           
  return this;
}

NetZone* NetZone::set_loopback_cb(const std::function<Link*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb)
{
  xbt_assert(dynamic_cast<kernel::routing::ClusterBase*>(get_impl()) != nullptr, 
             "set_loopback_cb can only be called for ClusterZones");
  kernel::actor::simcall_answered([this, &cb]() { static_cast<kernel::routing::ClusterBase*>(pimpl_)->set_loopback_cb(cb); });           
  return this;
}
NetZone* NetZone::set_limiter_cb(const std::function<Link*(NetZone* zone, const std::vector<unsigned long>& coord, unsigned long id)>& cb)
{
  xbt_assert(dynamic_cast<kernel::routing::ClusterBase*>(get_impl()) != nullptr, 
             "set_loopback_cb can only be called for ClusterZones");
  kernel::actor::simcall_answered([this, &cb]() { static_cast<kernel::routing::ClusterBase*>(pimpl_)->set_limiter_cb(cb); });           
  return this;
}
  


NetZone* NetZone::add_netzone_full(const std::string& name)
{
  auto* res = new kernel::routing::FullZone(name);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_star(const std::string& name)
{
  auto* res = new kernel::routing::StarZone(name);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_dijkstra(const std::string& name, bool cache)
{
  auto* res = new kernel::routing::DijkstraZone(name, cache);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_empty(const std::string& name)
{
  auto* res = new kernel::routing::EmptyZone(name);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_floyd(const std::string& name)
{
  auto* res = new kernel::routing::FloydZone(name);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_vivaldi(const std::string& name)
{
  auto* res = new kernel::routing::VivaldiZone(name);
  return res->set_parent(get_impl())->get_iface();
}
NetZone* NetZone::add_netzone_wifi(const std::string& name)
{
  auto* res = new kernel::routing::WifiZone(name);
  return res->set_parent(get_impl())->get_iface();
}

s4u::Host* NetZone::add_host(const std::string& name, double speed)
{
  return add_host(name, std::vector<double>{speed});
}

s4u::Host* NetZone::add_host(const std::string& name, const std::vector<double>& speed_per_pstate)
{
  return kernel::actor::simcall_answered(
      [this, &name, &speed_per_pstate] { return pimpl_->add_host(name, speed_per_pstate); });
}

s4u::Host* NetZone::add_host(const std::string& name, const std::string& speed)
{
  return add_host(name, std::vector<std::string>{speed});
}

s4u::Host* NetZone::add_host(const std::string& name, const std::vector<std::string>& speed_per_pstate)
{
  return add_host(name, Host::convert_pstate_speed_vector(speed_per_pstate));
}

s4u::Link* NetZone::add_link(const std::string& name, double bandwidth)
{
  return add_link(name, std::vector<double>{bandwidth});
}

s4u::Link* NetZone::add_link(const std::string& name, const std::vector<double>& bandwidths)
{
  return kernel::actor::simcall_answered([this, &name, &bandwidths] { return pimpl_->add_link(name, bandwidths); });
}

s4u::Link* NetZone::add_link(const std::string& name, const std::string& bandwidth)
{
  return add_link(name, std::vector<std::string>{bandwidth});
}

s4u::SplitDuplexLink* NetZone::add_split_duplex_link(const std::string& name, const std::string& str_up,
                                                        const std::string& str_down)
{
  double bw_up, bw_down;

  try {
    bw_up = xbt_parse_get_bandwidth("", 0, str_up, "");
  } catch (const simgrid::ParseError&) {
    throw std::invalid_argument("Impossible to create split-duplex link: " + name + ". Invalid bandwidths: " + str_up);
  }
  try {
    bw_down = str_down == "" ? bw_up : xbt_parse_get_bandwidth("", 0, str_down, "");
  } catch (const simgrid::ParseError&) {
    throw std::invalid_argument("Impossible to create split-duplex link: " + name +
                                ". Invalid bandwidths: " + str_down);
  }
  return add_split_duplex_link(name, bw_up, bw_down);
}

s4u::SplitDuplexLink* NetZone::add_split_duplex_link(const std::string& name, double bw_up, double bw_down)
{
  if (bw_down < 0)
    bw_down = bw_up;
  return kernel::actor::simcall_answered([this, &name, &bw_up, &bw_down] {
    return pimpl_->add_split_duplex_link(name, std::vector<double>{bw_up}, std::vector<double>{bw_down});
  });
}

s4u::Link* NetZone::add_link(const std::string& name, const std::vector<std::string>& bandwidths)
{
  std::vector<double> bw;
  bw.reserve(bandwidths.size());
  for (const auto& speed_str : bandwidths) {
    try {
      double speed = xbt_parse_get_bandwidth("", 0, speed_str, "");
      bw.push_back(speed);
    } catch (const simgrid::ParseError&) {
      throw std::invalid_argument("Impossible to create link: " + name + ". Invalid bandwidth: " + speed_str);
    }
  }
  return add_link(name, bw);
}

kernel::routing::NetPoint* NetZone::add_router(const std::string& name)
{
  return kernel::actor::simcall_answered([this, &name] { return pimpl_->add_router(name); });
}

kernel::routing::NetPoint* NetZone::get_netpoint() const
{
  return pimpl_->get_netpoint();
}

kernel::routing::NetPoint* NetZone::get_gateway() const
{
  return pimpl_->get_gateway();
}

kernel::routing::NetPoint* NetZone::get_gateway(const std::string& name) const
{
  return pimpl_->get_gateway(name);
}

void NetZone::set_gateway(kernel::routing::NetPoint* router)
{
  set_gateway("default", router);
}

void NetZone::set_gateway(const std::string& name, kernel::routing::NetPoint* router)
{
  kernel::actor::simcall_answered([this, name, router] { pimpl_->set_gateway(name, router); });
}

kernel::resource::NetworkModel* NetZone::get_network_model() const
{
  return pimpl_->get_network_model().get();
}
} // namespace simgrid::s4u

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
sg_netzone_t* sg_zone_get_childs(const_sg_netzone_t netzone, int* size)
{
  auto children     = netzone->get_children();
  if (size)
    *size = children.size();
  sg_netzone_t* res = (sg_netzone_t*)xbt_malloc(sizeof(char*) * (children.size() + 1));
  int i             = 0;
  for (auto child : children)
    res[i++] = child;
  res[i] = nullptr;

  return res;
}
const char** sg_zone_get_property_names(const_sg_netzone_t zone)
{
  const std::unordered_map<std::string, std::string>* props = zone->get_properties();

  if (props == nullptr)
    return nullptr;

  const char** res = (const char**)xbt_malloc(sizeof(char*) * (props->size() + 1));
  int i            = 0;
  for (auto const& [key, _] : *props)
    res[i++] = key.c_str();
  res[i] = nullptr;

  return res;
}
const char* sg_zone_get_property_value(const_sg_netzone_t netzone, const char* name)
{
  return netzone->get_property(name);
}

void sg_zone_set_property_value(sg_netzone_t netzone, const char* name, const char* value)
{
  netzone->set_property(name, value);
}

void sg_zone_get_hosts(const_sg_netzone_t netzone, xbt_dynar_t whereto) // deprecate 3.39
{
  /* converts vector to dynar */
  std::vector<simgrid::s4u::Host*> hosts = netzone->get_all_hosts();
  for (auto const& host : hosts)
    xbt_dynar_push(whereto, &host);
}
const_sg_host_t* sg_zone_get_all_hosts(const_sg_netzone_t zone, int* size)
{
  std::vector<simgrid::s4u::Host*> hosts = zone->get_all_hosts();

  const_sg_host_t* res = (const_sg_host_t*)xbt_malloc(sizeof(const_sg_link_t) * (hosts.size() + 1));
  if (size)
    *size = hosts.size();
  int i = 0;
  for (auto host : hosts)
    res[i++] = host;
  res[i] = nullptr;

  return res;
}
