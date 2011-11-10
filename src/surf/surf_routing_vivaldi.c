/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");

/* Business methods */
static route_extended_t vivaldi_get_route(AS_t rc,
                                            const char *src,
                                            const char *dst)
{
	  xbt_assert(rc && src
	              && dst,
	              "Invalid params for \"get_route\" function at AS \"%s\"",
	              rc->name);

	  route_extended_t new_e_route = xbt_new0(s_route_extended_t, 1);
	  new_e_route->src_gateway = ROUTER_PEER(src);
	  new_e_route->dst_gateway = ROUTER_PEER(dst);
	  new_e_route->generic_route.link_list =  xbt_dynar_new(0, NULL);
	  return new_e_route;
}

static double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst)
{
	double src_coord, dst_coord;

	src_coord = atof(xbt_dynar_get_as(src, index, char *));
	dst_coord = atof(xbt_dynar_get_as(dst, index, char *));

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
  				 + fabs(atof(xbt_dynar_get_as(src_ctn, 2, char *)))+fabs(atof(xbt_dynar_get_as(dst_ctn, 2, char *)));

  xbt_assert(euclidean_dist>=0, "Euclidean Dist is less than 0\"%s\" and \"%.2f\"", src, euclidean_dist);

  //From .ms to .s
  return euclidean_dist / 1000;
}

static double vivaldi_get_link_latency (AS_t rc,const char *src, const char *dst, route_extended_t e_route)
{
  if(routing_get_network_element_type(src) == SURF_NETWORK_ELEMENT_AS) {
	  int need_to_clean = e_route?0:1;
	  double latency;
	  e_route = e_route ? e_route : rc->get_route(rc, src, dst);
	  latency = base_vivaldi_get_latency(e_route->src_gateway,e_route->dst_gateway);
	  if(need_to_clean) generic_free_extended_route(e_route);
	  return latency;
  } else {
	  return base_vivaldi_get_latency(src,dst);
  }
}

/* Creation routing model functions */
AS_t model_vivaldi_create(void)
{
	  AS_t new_component = model_none_create();
	  new_component->get_route = vivaldi_get_route;
	  new_component->get_latency = vivaldi_get_link_latency;
	  return new_component;
}
