/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <boost/algorithm/string.hpp>

#include <simgrid/s4u/host.hpp>

#include "src/kernel/routing/NetCard.hpp"
#include "src/kernel/routing/VivaldiZone.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {
namespace vivaldi {
simgrid::xbt::Extension<NetCard, Coords> Coords::EXTENSION_ID;

Coords::Coords(NetCard* netcard, const char* coordStr)
{
  if (!Coords::EXTENSION_ID.valid())
    Coords::EXTENSION_ID = NetCard::extension_create<Coords>();

  std::vector<std::string> string_values;
  boost::split(string_values, coordStr, boost::is_any_of(" "));
  xbt_assert(string_values.size() == 3, "Coordinates of %s must have 3 dimensions", netcard->cname());

  for (auto str : string_values)
    coords.push_back(xbt_str_parse_double(str.c_str(), "Invalid coordinate: %s"));
  coords.shrink_to_fit();

  netcard->extension_set<Coords>(this);
  XBT_DEBUG("Coords of %s %p: %s", netcard->cname(), netcard, coordStr);
}
Coords::~Coords() = default;
}; // namespace vivaldi

static inline double euclidean_dist_comp(int index, std::vector<double>* src, std::vector<double>* dst)
{
  double src_coord = src->at(index);
  double dst_coord = dst->at(index);

  return (src_coord - dst_coord) * (src_coord - dst_coord);
}

static std::vector<double>* getCoordsFromNetcard(NetCard* nc)
{
  simgrid::kernel::routing::vivaldi::Coords* coords = nc->extension<simgrid::kernel::routing::vivaldi::Coords>();
  xbt_assert(coords, "Please specify the Vivaldi coordinates of %s %s (%p)",
             (nc->isNetZone() ? "AS" : (nc->isHost() ? "Host" : "Router")), nc->cname(), nc);
  return &coords->coords;
}
VivaldiZone::VivaldiZone(NetZone* father, const char* name) : ClusterZone(father, name)
{
}

void VivaldiZone::setPeerLink(NetCard* netcard, double bw_in, double bw_out, double latency, const char* coord)
{
  xbt_assert(netcard->netzone() == this, "Cannot add a peer link to a netcard that is not in this AS");

  new simgrid::kernel::routing::vivaldi::Coords(netcard, coord);

  char* link_up   = bprintf("link_%s_UP", netcard->cname());
  char* link_down = bprintf("link_%s_DOWN", netcard->cname());
  Link* linkUp    = surf_network_model->createLink(link_up, bw_out, latency, SURF_LINK_SHARED);
  Link* linkDown  = surf_network_model->createLink(link_down, bw_in, latency, SURF_LINK_SHARED);
  privateLinks_.insert({netcard->id(), {linkUp, linkDown}});

  free(link_up);
  free(link_down);
}

void VivaldiZone::getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t route, double* lat)
{
  XBT_DEBUG("vivaldi getLocalRoute from '%s'[%d] '%s'[%d]", src->cname(), src->id(), dst->cname(), dst->id());

  if (src->isNetZone()) {
    char* srcName = bprintf("router_%s", src->cname());
    char* dstName = bprintf("router_%s", dst->cname());
    route->gw_src = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib, srcName, ROUTING_ASR_LEVEL);
    route->gw_dst = (sg_netcard_t)xbt_lib_get_or_null(as_router_lib, dstName, ROUTING_ASR_LEVEL);
    xbt_free(srcName);
    xbt_free(dstName);
  }

  /* Retrieve the private links */
  if (privateLinks_.find(src->id()) != privateLinks_.end()) {
    std::pair<Link*, Link*> info = privateLinks_.at(src->id());
    if (info.first) {
      route->link_list->push_back(info.first);
      if (lat)
        *lat += info.first->latency();
    }
  }
  if (privateLinks_.find(dst->id()) != privateLinks_.end()) {
    std::pair<Link*, Link*> info = privateLinks_.at(dst->id());
    if (info.second) {
      route->link_list->push_back(info.second);
      if (lat)
        *lat += info.second->latency();
    }
  }

  /* Compute the extra latency due to the euclidean distance if needed */
  if (lat) {
    std::vector<double>* srcCoords = getCoordsFromNetcard(src);
    std::vector<double>* dstCoords = getCoordsFromNetcard(dst);

    double euclidean_dist =
        sqrt(euclidean_dist_comp(0, srcCoords, dstCoords) + euclidean_dist_comp(1, srcCoords, dstCoords)) +
        fabs(srcCoords->at(2)) + fabs(dstCoords->at(2));

    XBT_DEBUG("Updating latency %f += %f", *lat, euclidean_dist);
    *lat += euclidean_dist / 1000.0; // From .ms to .s
  }
}
}
}
}
