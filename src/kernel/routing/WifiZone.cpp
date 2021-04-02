/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/WifiZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

#include <unordered_set>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_wifi, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

void WifiZone::do_seal()
{
  const char* AP_name = get_property("access_point");
  if (AP_name != nullptr) {
    access_point_ = sg_netpoint_by_name_or_null(AP_name);
    xbt_assert(access_point_ != nullptr, "Access point '%s' of WIFI zone '%s' does not exist: no such host or router.",
               AP_name, get_cname());
    xbt_assert(access_point_->is_host() || access_point_->is_router(),
               "Access point '%s' of WIFI zone '%s' must be either a host or a router.", AP_name, get_cname());
  }
}

void WifiZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* res, double* lat)
{
  XBT_DEBUG("full getLocalRoute from %s[%u] to %s[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  if (wifi_link_ != nullptr) {
    // If src and dst are nodes, not access_point, we need to traverse the link twice
    // Otherwise (if src or dst is access_point), we need to traverse the link only once

    if (src != access_point_) {
      XBT_DEBUG("src %s is not our gateway", src->get_cname());
      res->link_list.push_back(wifi_link_);
      if (lat)
        *lat += wifi_link_->get_latency();
    }
    if (dst != access_point_) {
      XBT_DEBUG("dst %s is not our gateway", dst->get_cname());
      res->link_list.push_back(wifi_link_);
      if (lat)
        *lat += wifi_link_->get_latency();
    }
  }
}

s4u::Link* WifiZone::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  xbt_assert(wifi_link_ == nullptr,
             "WIFI netzone %s contains more than one link. Please only declare one, the wifi link.", get_cname());

  auto s4u_link = NetZoneImpl::create_wifi_link(name, bandwidths)->set_sharing_policy(s4u::Link::SharingPolicy::WIFI);
  wifi_link_    = s4u_link->get_impl();
  return s4u_link;
}
} // namespace routing
} // namespace kernel

namespace s4u {
NetZone* create_wifi_zone(const std::string& name)
{
  return (new kernel::routing::WifiZone(name))->get_iface();
}
} // namespace s4u

} // namespace simgrid
