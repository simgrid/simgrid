/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

/* Business methods */
static void vivaldi_get_route(AS_t rc, const char *src, const char *dst,
    route_t route)
{
	  xbt_assert(rc && src
	              && dst,
	              "Invalid params for \"get_route\" function at AS \"%s\"",
	              rc->name);

	  route->src_gateway = ROUTER_PEER(src);
	  route->dst_gateway = ROUTER_PEER(dst);
}

static XBT_INLINE double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst)
{
	double src_coord, dst_coord;

  src_coord = xbt_dynar_get_as(src, index, double);
  dst_coord = xbt_dynar_get_as(dst, index, double);

	return (src_coord-dst_coord)*(src_coord-dst_coord);

}

static double base_vivaldi_get_latency (const char *src, const char *dst)
{
  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  src_ctn = xbt_lib_get_or_null(host_lib, src, COORD_HOST_LEVEL);
  if(!src_ctn) src_ctn = xbt_lib_get_or_null(as_router_lib, src, COORD_ASR_LEVEL);
  dst_ctn = xbt_lib_get_or_null(host_lib, dst, COORD_HOST_LEVEL);
  if(!dst_ctn) dst_ctn = xbt_lib_get_or_null(as_router_lib, dst, COORD_ASR_LEVEL);

  if(dst_ctn == NULL || src_ctn == NULL)
  xbt_die("Coord src '%s' :%p   dst '%s' :%p",src,src_ctn,dst,dst_ctn);

  euclidean_dist = sqrt (euclidean_dist_comp(0,src_ctn,dst_ctn)+euclidean_dist_comp(1,src_ctn,dst_ctn))
  				 + fabs(xbt_dynar_get_as(src_ctn, 2, double))+fabs(xbt_dynar_get_as(dst_ctn, 2, double));

  //From .ms to .s
  return euclidean_dist / 1000;
}

static double vivaldi_get_latency (AS_t rc,const char *src, const char *dst, route_t e_route)
{
  if(routing_get_network_element_type(src) == SURF_NETWORK_ELEMENT_AS) {
    if (e_route)
      return base_vivaldi_get_latency(e_route->src_gateway,e_route->dst_gateway);
    return base_vivaldi_get_latency(ROUTER_PEER(src),ROUTER_PEER(dst));
  } else {
	  return base_vivaldi_get_latency(src,dst);
  }
}

/* Creation routing model functions */
AS_t model_vivaldi_create(void)
{
	  AS_t new_component = model_rulebased_create();
	  new_component->get_route = vivaldi_get_route;
	  new_component->get_latency = vivaldi_get_latency;
	  return new_component;
}
