/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

static XBT_INLINE double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst) {
  double src_coord, dst_coord;

  src_coord = xbt_dynar_get_as(src, index, double);
  dst_coord = xbt_dynar_get_as(dst, index, double);

  return (src_coord-dst_coord)*(src_coord-dst_coord);
}

static void vivaldi_get_route_and_latency(
    AS_t rc, network_element_t src_p, network_element_t dst_p,
    route_t route, double *lat)
{
  XBT_DEBUG("vivaldi_get_route_and_latency from '%s'[%d] '%s'[%d]",src_p->name,src_p->id,dst_p->name,dst_p->id);
  char *src = (char*)src_p->name;
  char *dst = (char*)dst_p->name;

  if(routing_get_network_element_type(src) == SURF_NETWORK_ELEMENT_AS) {
    route->src_gateway = xbt_lib_get_or_null(as_router_lib,ROUTER_PEER(src),ROUTING_ASR_LEVEL);
    route->dst_gateway = xbt_lib_get_or_null(as_router_lib,ROUTER_PEER(dst),ROUTING_ASR_LEVEL);
  }

  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  char *tmp_src_name, *tmp_dst_name;

  if(src_p->rc_type == SURF_NETWORK_ELEMENT_HOST){
    tmp_src_name = HOST_PEER(src);
    src_ctn = xbt_lib_get_or_null(host_lib, tmp_src_name, COORD_HOST_LEVEL);
    if(!src_ctn ) src_ctn = xbt_lib_get_or_null(host_lib, src, COORD_HOST_LEVEL);
  }
  else if(src_p->rc_type == SURF_NETWORK_ELEMENT_ROUTER || src_p->rc_type == SURF_NETWORK_ELEMENT_AS){
    tmp_src_name = ROUTER_PEER(src);
    src_ctn = xbt_lib_get_or_null(as_router_lib, tmp_src_name, COORD_ASR_LEVEL);
  }
  else{
    xbt_die(" ");
  }

  if(dst_p->rc_type == SURF_NETWORK_ELEMENT_HOST){
    tmp_dst_name = HOST_PEER(dst);
    dst_ctn = xbt_lib_get_or_null(host_lib, tmp_dst_name, COORD_HOST_LEVEL);
    if(!dst_ctn ) dst_ctn = xbt_lib_get_or_null(host_lib, dst, COORD_HOST_LEVEL);
  }
  else if(dst_p->rc_type == SURF_NETWORK_ELEMENT_ROUTER || dst_p->rc_type == SURF_NETWORK_ELEMENT_AS){
    tmp_dst_name = ROUTER_PEER(dst);
    dst_ctn = xbt_lib_get_or_null(as_router_lib, tmp_dst_name, COORD_ASR_LEVEL);
  }
  else{
    xbt_die(" ");
  }

  xbt_assert(src_ctn,"No coordinate found for element '%s'",tmp_src_name);
  xbt_assert(dst_ctn,"No coordinate found for element '%s'",tmp_dst_name);
  free(tmp_src_name);
  free(tmp_dst_name);

  euclidean_dist = sqrt (euclidean_dist_comp(0,src_ctn,dst_ctn)+euclidean_dist_comp(1,src_ctn,dst_ctn))
                  + fabs(xbt_dynar_get_as(src_ctn, 2, double))+fabs(xbt_dynar_get_as(dst_ctn, 2, double));

  if (lat)
    *lat += euclidean_dist / 1000; //From .ms to .s
}


/* Creation routing model functions */
AS_t model_vivaldi_create(void)
{
	  AS_t new_component = model_rulebased_create();
	  new_component->get_route_and_latency = vivaldi_get_route_and_latency;
	  return new_component;
}
