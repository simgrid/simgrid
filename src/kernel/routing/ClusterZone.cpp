/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include <tuple>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_routing_cluster, ker_platform, "Kernel Cluster Routing");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnection with the rest of the world. */

namespace simgrid::kernel::routing {

void ClusterBase::set_loopback()
{
  if (not has_loopback_) {
    num_links_per_node_++;
    has_loopback_ = true;
  }
}

void ClusterBase::set_limiter()
{
  if (not has_limiter_) {
    num_links_per_node_++;
    has_limiter_ = true;
  }
}
 
void ClusterBase::set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy)
{
  link_sharing_policy_ = sharing_policy;
  link_bw_             = bw;
  link_lat_            = lat;
}

void ClusterBase::add_private_link_at(unsigned long position,
                                      std::pair<resource::StandardLinkImpl*, resource::StandardLinkImpl*> link)
{
  private_links_.try_emplace(position, link);
}

void ClusterBase::set_gateway(unsigned long position, NetPoint* gateway)
{
  xbt_assert(not gateway || not gateway->is_netzone(), "ClusterBase: gateway cannot be another netzone %s",
             gateway->get_cname());
  gateways_[position] = gateway;
}

NetPoint* ClusterBase::get_gateway(unsigned long position)
{
  auto it = gateways_.find(position);
  return it == gateways_.end() ? nullptr : it->second;
}


std::tuple<NetPoint*, s4u::Link*, s4u::Link*> ClusterBase::fill_leaf_from_cb(unsigned long position)
{
  s4u::Link* loopback_res = nullptr;
  s4u::Link* limiter_res  = nullptr;

  // auxiliary function to get dims from index
  auto index_to_dims = [this](unsigned long index) {
    std::vector<unsigned long> dims_array(dims_.size());
    for (auto i = static_cast<int>(dims_.size() - 1); i >= 0; --i) {
      if (index == 0)
        break;
      unsigned long value = index % dims_[i];
      dims_array[i]      = value;
      index              = (index / dims_[i]);
    }
    return dims_array;
  };

  kernel::routing::NetPoint* netpoint = nullptr;
  kernel::routing::NetPoint* gw       = nullptr;
  auto dims                           = index_to_dims(position);
  if (netzone_cb_) {
    s4u::NetZone* netzone = netzone_cb_(get_iface(), dims, position);
    netpoint              = netzone->get_netpoint();
    gw                    = netzone->get_gateway();
  } else {
    s4u::Host* host = host_cb_(get_iface(), dims, position);
    netpoint        = host->get_netpoint();
  }

  xbt_assert(netpoint, "set_netpoint(elem=%lu): Invalid netpoint (nullptr)", position);
  if (netpoint->is_netzone()) {
    xbt_assert(gw && not gw->is_netzone(),
               "set_netpoint(elem=%lu): Netpoint (%s) is a netzone, but gateway (%s) is invalid", position,
               netpoint->get_cname(), gw ? gw->get_cname() : "nullptr");
  } else {
    xbt_assert(not gw, "set_netpoint: Netpoint (%s) isn't netzone, gateway must be nullptr", netpoint->get_cname());
  }
  // setting gateway
  set_gateway(position, gw);

  if (loopback_cb_) {
    s4u::Link* loopback = loopback_cb_(get_iface(), dims, position);
    xbt_assert(loopback, "set_loopback: Invalid loopback link (nullptr) for element %lu", position);
    set_loopback();
    add_private_link_at(node_pos(netpoint->id()), {loopback->get_impl(), loopback->get_impl()});
    loopback_res = loopback;
  }

  if (limiter_cb_) {
    s4u::Link* limiter = limiter_cb_(get_iface(), dims, position);
    xbt_assert(limiter, "set_limiter: Invalid limiter link (nullptr) for element %lu", position);
    set_limiter();
    add_private_link_at(node_pos_with_loopback(netpoint->id()), {limiter->get_impl(), limiter->get_impl()});
    limiter_res = limiter;
  }
  return std::make_tuple(netpoint, loopback_res, limiter_res);
}

} // namespace simgrid::kernel::routing
