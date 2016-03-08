/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/AsVivaldi.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

#define HOST_PEER(peername) bprintf("peer_%s", peername)
#define ROUTER_PEER(peername) bprintf("router_%s", peername)
#define LINK_PEER(peername) bprintf("link_%s", peername)

static inline double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst) {
  double src_coord, dst_coord;

  src_coord = xbt_dynar_get_as(src, index, double);
  dst_coord = xbt_dynar_get_as(dst, index, double);

  return (src_coord-dst_coord)*(src_coord-dst_coord);
}

namespace simgrid {
namespace surf {
  AsVivaldi::AsVivaldi(const char *name)
    : AsRoutedGraph(name)
  {}

void AsVivaldi::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t route, double *lat)
{
  s_surf_parsing_link_up_down_t info;

  XBT_DEBUG("vivaldi_get_route_and_latency from '%s'[%d] '%s'[%d]",
      src->name(), src->id(), dst->name(), dst->id());

  if(src->isAS()) {
    char *src_name = ROUTER_PEER(src->name());
    char *dst_name = ROUTER_PEER(dst->name());
    route->gw_src = (sg_netcard_t) xbt_lib_get_or_null(as_router_lib, src_name, ROUTING_ASR_LEVEL);
    route->gw_dst = (sg_netcard_t) xbt_lib_get_or_null(as_router_lib, dst_name, ROUTING_ASR_LEVEL);
    xbt_free(src_name);
    xbt_free(dst_name);
  }

  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  char *tmp_src_name, *tmp_dst_name;

  if(src->isHost()){
    tmp_src_name = HOST_PEER(src->name());

    if ((int)xbt_dynar_length(upDownLinks)>src->id()) {
      info = xbt_dynar_get_as(upDownLinks, src->id(), s_surf_parsing_link_up_down_t);
      if(info.link_up) { // link up
        route->link_list->push_back(info.link_up);
        if (lat)
          *lat += static_cast<Link*>(info.link_up)->getLatency();
      }
    }
    src_ctn = (xbt_dynar_t) simgrid::s4u::Host::by_name_or_create(tmp_src_name)->extension(COORD_HOST_LEVEL);
    if (src_ctn == nullptr)
      src_ctn = (xbt_dynar_t) simgrid::s4u::Host::by_name_or_create(src->name())->extension(COORD_HOST_LEVEL);
  }
  else if(src->isRouter() || src->isAS()){
    tmp_src_name = ROUTER_PEER(src->name());
    src_ctn = (xbt_dynar_t) xbt_lib_get_or_null(as_router_lib, tmp_src_name, COORD_ASR_LEVEL);
  }
  else{
    THROW_IMPOSSIBLE;
  }

  if(dst->isHost()){
    tmp_dst_name = HOST_PEER(dst->name());

    if ((int)xbt_dynar_length(upDownLinks)>dst->id()) {
      info = xbt_dynar_get_as(upDownLinks, dst->id(), s_surf_parsing_link_up_down_t);
      if(info.link_down) { // link down
        route->link_list->push_back(info.link_down);
        if (lat)
          *lat += static_cast<Link*>(info.link_down)->getLatency();
      }
    }
    dst_ctn = (xbt_dynar_t) simgrid::s4u::Host::by_name_or_create(tmp_dst_name)
      ->extension(COORD_HOST_LEVEL);
    if (dst_ctn == nullptr)
      dst_ctn = (xbt_dynar_t) simgrid::s4u::Host::by_name_or_create(dst->name())
        ->extension(COORD_HOST_LEVEL);
  }
  else if(dst->isRouter() || dst->isAS()){
    tmp_dst_name = ROUTER_PEER(dst->name());
    dst_ctn = (xbt_dynar_t) xbt_lib_get_or_null(as_router_lib, tmp_dst_name, COORD_ASR_LEVEL);
  }
  else{
    THROW_IMPOSSIBLE;
  }

  xbt_assert(src_ctn,"No coordinate found for element '%s'",tmp_src_name);
  xbt_assert(dst_ctn,"No coordinate found for element '%s'",tmp_dst_name);
  free(tmp_src_name);
  free(tmp_dst_name);

  euclidean_dist = sqrt (euclidean_dist_comp(0,src_ctn,dst_ctn)+euclidean_dist_comp(1,src_ctn,dst_ctn))
                      + fabs(xbt_dynar_get_as(src_ctn, 2, double))+fabs(xbt_dynar_get_as(dst_ctn, 2, double));

  if (lat){
    XBT_DEBUG("Updating latency %f += %f",*lat,euclidean_dist);
    *lat += euclidean_dist / 1000.0; //From .ms to .s
  }
}

}
}
