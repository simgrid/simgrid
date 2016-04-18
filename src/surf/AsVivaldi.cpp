/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/AsVivaldi.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

namespace simgrid {
namespace surf {
  static inline double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst) {
    double src_coord = xbt_dynar_get_as(src, index, double);
    double dst_coord = xbt_dynar_get_as(dst, index, double);

    return (src_coord-dst_coord)*(src_coord-dst_coord);
  }

  static xbt_dynar_t getCoordsFromNetcard(NetCard *nc)
  {
    xbt_dynar_t res = nullptr;
    char *tmp_name;

    if(nc->isHost()){
      tmp_name = bprintf("peer_%s", nc->name());
      simgrid::s4u::Host *host = simgrid::s4u::Host::by_name_or_null(tmp_name);
      if (host == nullptr)
        host = simgrid::s4u::Host::by_name_or_null(nc->name());
      if (host != nullptr)
        res = (xbt_dynar_t) host->extension(COORD_HOST_LEVEL);
    }
    else if(nc->isRouter() || nc->isAS()){
      tmp_name = bprintf("router_%s", nc->name());
      res = (xbt_dynar_t) xbt_lib_get_or_null(as_router_lib, tmp_name, COORD_ASR_LEVEL);
    }
    else{
      THROW_IMPOSSIBLE;
    }

    xbt_assert(res,"No coordinate found for element '%s'",tmp_name);
    free(tmp_name);

    return res;
  }
  AsVivaldi::AsVivaldi(const char *name)
    : AsCluster(name)
  {}

void AsVivaldi::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t route, double *lat)
{
  XBT_DEBUG("vivaldi_get_route_and_latency from '%s'[%d] '%s'[%d]", src->name(), src->id(), dst->name(), dst->id());

  if(src->isAS()) {
    char *src_name = bprintf("router_%s",src->name());
    char *dst_name = bprintf("router_%s",dst->name());
    route->gw_src = (sg_netcard_t) xbt_lib_get_or_null(as_router_lib, src_name, ROUTING_ASR_LEVEL);
    route->gw_dst = (sg_netcard_t) xbt_lib_get_or_null(as_router_lib, dst_name, ROUTING_ASR_LEVEL);
    xbt_free(src_name);
    xbt_free(dst_name);
  }

  /* Retrieve the private links */
  if ((int)xbt_dynar_length(privateLinks_) > src->id()) {
    s_surf_parsing_link_up_down_t info = xbt_dynar_get_as(privateLinks_, src->id(), s_surf_parsing_link_up_down_t);
    if(info.link_up) {
      route->link_list->push_back(info.link_up);
      if (lat)
        *lat += info.link_up->getLatency();
    }
  }
  if ((int)xbt_dynar_length(privateLinks_)>dst->id()) {
    s_surf_parsing_link_up_down_t info = xbt_dynar_get_as(privateLinks_, dst->id(), s_surf_parsing_link_up_down_t);
    if(info.link_down) {
      route->link_list->push_back(info.link_down);
      if (lat)
        *lat += info.link_down->getLatency();
    }
  }

  /* Compute the extra latency due to the euclidean distance if needed */
  if (lat){
    xbt_dynar_t srcCoords = getCoordsFromNetcard(src);
    xbt_dynar_t dstCoords = getCoordsFromNetcard(dst);

    double euclidean_dist = sqrt (euclidean_dist_comp(0,srcCoords,dstCoords)+euclidean_dist_comp(1,srcCoords,dstCoords))
                              + fabs(xbt_dynar_get_as(srcCoords, 2, double))+fabs(xbt_dynar_get_as(dstCoords, 2, double));

    XBT_DEBUG("Updating latency %f += %f",*lat,euclidean_dist);
    *lat += euclidean_dist / 1000.0; //From .ms to .s
  }
}

}
}
