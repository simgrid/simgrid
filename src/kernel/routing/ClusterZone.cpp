/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnection with the rest of the world. */

namespace simgrid {
namespace kernel {
namespace routing {

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

void ClusterBase::add_private_link_at(unsigned long position, std::pair<resource::LinkImpl*, resource::LinkImpl*> link)
{
  private_links_.insert({position, link});
}

void ClusterBase::set_gateway(unsigned long position, NetPoint* gateway)
{
  xbt_assert(not gateway || not gateway->is_netzone(), "ClusterBase: gateway cannot be another netzone %s",
             gateway->get_cname());
  gateways_[position] = gateway;
}

NetPoint* ClusterBase::get_gateway(unsigned long position)
{
  NetPoint* res = nullptr;
  auto it       = gateways_.find(position);
  if (it != gateways_.end()) {
    res = it->second;
  }
  return res;
}

void ClusterBase::fill_leaf_from_cb(unsigned long position, const std::vector<unsigned long>& dimensions,
                                    const s4u::ClusterCallbacks& set_callbacks, NetPoint** node_netpoint,
                                    s4u::Link** lb_link, s4u::Link** limiter_link)
{
  xbt_assert(node_netpoint, "Invalid node_netpoint parameter");
  xbt_assert(lb_link, "Invalid lb_link parameter");
  xbt_assert(limiter_link, "Invalid limiter_link paramater");
  *lb_link      = nullptr;
  *limiter_link = nullptr;

  // auxiliary function to get dims from index
  auto index_to_dims = [&dimensions](unsigned long index) {
    std::vector<unsigned long> dims_array(dimensions.size());
    for (auto i = static_cast<int>(dimensions.size() - 1); i >= 0; --i) {
      if (index <= 0) {
        break;
      }
      unsigned long value = index % dimensions[i];
      dims_array[i]      = value;
      index              = (index / dimensions[i]);
    }
    return dims_array;
  };

  kernel::routing::NetPoint* netpoint = nullptr;
  kernel::routing::NetPoint* gw       = nullptr;
  auto dims                           = index_to_dims(position);
  std::tie(netpoint, gw)              = set_callbacks.netpoint(get_iface(), dims, position);
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

  if (set_callbacks.loopback) {
    s4u::Link* loopback = set_callbacks.loopback(get_iface(), dims, position);
    xbt_assert(loopback, "set_loopback: Invalid loopback link (nullptr) for element %lu", position);
    set_loopback();
    add_private_link_at(node_pos(netpoint->id()), {loopback->get_impl(), loopback->get_impl()});
    *lb_link = loopback;
  }

  if (set_callbacks.limiter) {
    s4u::Link* limiter = set_callbacks.limiter(get_iface(), dims, position);
    xbt_assert(limiter, "set_limiter: Invalid limiter link (nullptr) for element %lu", position);
    set_limiter();
    add_private_link_at(node_pos_with_loopback(netpoint->id()), {limiter->get_impl(), limiter->get_impl()});
    *limiter_link = limiter;
  }
  *node_netpoint = netpoint;
}

} // namespace routing
} // namespace kernel
} // namespace simgrid
