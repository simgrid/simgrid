/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/VivaldiZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

#include <boost/algorithm/string.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

namespace vivaldi {

xbt::Extension<NetPoint, Coords> Coords::EXTENSION_ID;

Coords::Coords(NetPoint* netpoint, const std::string& coordStr)
{
  if (not Coords::EXTENSION_ID.valid())
    Coords::EXTENSION_ID = NetPoint::extension_create<Coords>();

  std::vector<std::string> string_values;
  boost::split(string_values, coordStr, boost::is_any_of(" "));
  xbt_assert(string_values.size() == 3, "Coordinates of %s must have 3 dimensions", netpoint->get_cname());

  for (auto const& str : string_values)
    try {
      coords.push_back(std::stod(str));
    } catch (std::invalid_argument const& ia) {
      throw std::invalid_argument(std::string("Invalid coordinate: ") + ia.what());
    }
  coords.shrink_to_fit();

  netpoint->extension_set<Coords>(this);
  XBT_DEBUG("Coords of %s %p: %s", netpoint->get_cname(), netpoint, coordStr.c_str());
}
} // namespace vivaldi

static inline double euclidean_dist_comp(int index, std::vector<double>* src, std::vector<double>* dst)
{
  double src_coord = src->at(index);
  double dst_coord = dst->at(index);

  return (src_coord - dst_coord) * (src_coord - dst_coord);
}

static std::vector<double>* netpoint_get_coords(NetPoint* np)
{
  auto* coords = np->extension<vivaldi::Coords>();
  xbt_assert(coords, "Please specify the Vivaldi coordinates of %s %s (%p)",
             (np->is_netzone() ? "Netzone" : (np->is_host() ? "Host" : "Router")), np->get_cname(), np);
  return &coords->coords;
}

VivaldiZone::VivaldiZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : ClusterZone(father, name, netmodel)
{
}

void VivaldiZone::set_peer_link(NetPoint* netpoint, double bw_in, double bw_out, const std::string& coord)
{
  xbt_assert(netpoint->get_englobing_zone() == this,
             "Cannot add a peer link to a netpoint that is not in this netzone");

  new vivaldi::Coords(netpoint, coord);

  std::string link_up      = "link_" + netpoint->get_name() + "_UP";
  std::string link_down    = "link_" + netpoint->get_name() + "_DOWN";
  resource::LinkImpl* linkUp =
      network_model_->create_link(link_up, std::vector<double>(1, bw_out), 0, s4u::Link::SharingPolicy::SHARED);
  resource::LinkImpl* linkDown =
      network_model_->create_link(link_down, std::vector<double>(1, bw_in), 0, s4u::Link::SharingPolicy::SHARED);
  private_links_.insert({netpoint->id(), {linkUp, linkDown}});
}

void VivaldiZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  XBT_DEBUG("vivaldi getLocalRoute from '%s'[%u] '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  if (src->is_netzone()) {
    std::string srcName = "router_" + src->get_name();
    std::string dstName = "router_" + dst->get_name();
    route->gw_src       = s4u::Engine::get_instance()->netpoint_by_name_or_null(srcName);
    route->gw_dst       = s4u::Engine::get_instance()->netpoint_by_name_or_null(dstName);
  }

  /* Retrieve the private links */
  auto src_link = private_links_.find(src->id());
  if (src_link != private_links_.end()) {
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = src_link->second;
    if (info.first) {
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->get_latency();
    }
  } else {
    XBT_DEBUG("Source of private link (%u) doesn't exist", src->id());
  }

  auto dst_link = private_links_.find(dst->id());
  if (dst_link != private_links_.end()) {
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = dst_link->second;
    if (info.second) {
      route->link_list.push_back(info.second);
      if (lat)
        *lat += info.second->get_latency();
    }
  } else {
    XBT_DEBUG("Destination of private link (%u) doesn't exist", dst->id());
  }

  /* Compute the extra latency due to the euclidean distance if needed */
  if (lat) {
    std::vector<double>* srcCoords = netpoint_get_coords(src);
    std::vector<double>* dstCoords = netpoint_get_coords(dst);

    double euclidean_dist =
        sqrt(euclidean_dist_comp(0, srcCoords, dstCoords) + euclidean_dist_comp(1, srcCoords, dstCoords)) +
        fabs(srcCoords->at(2)) + fabs(dstCoords->at(2));

    XBT_DEBUG("Updating latency %f += %f", *lat, euclidean_dist);
    *lat += euclidean_dist / 1000.0; // From .ms to .s
  }
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
