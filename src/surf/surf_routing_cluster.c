/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

/* Global vars */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnexion with the rest of the world.
 */

/* Business methods */
static void cluster_get_route_and_latency(AS_t as,
    sg_routing_edge_t src, sg_routing_edge_t dst,
                                          route_t route, double *lat) {

      s_surf_parsing_link_up_down_t info;
    XBT_DEBUG("cluster_get_route_and_latency from '%s'[%d] to '%s'[%d]",
        src->name,src->id,
        dst->name,dst->id);

    if(src->rc_type != SURF_NETWORK_ELEMENT_ROUTER){ // No specific link for router
        info = xbt_dynar_get_as(as->link_up_down_list,src->id,s_surf_parsing_link_up_down_t);
        if(info.link_up) { // link up
          xbt_dynar_push_as(route->link_list,void*,info.link_up);
        if (lat)
          *lat += surf_network_model->extension.network.get_link_latency(info.link_up);
        }
    }

    if ( ((as_cluster_t)as)->backbone ) {
      xbt_dynar_push_as(route->link_list,void*, ((as_cluster_t)as)->backbone) ;
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(((as_cluster_t)as)->backbone);
    }

    if(dst->rc_type != SURF_NETWORK_ELEMENT_ROUTER){ // No specific link for router
        info = xbt_dynar_get_as(as->link_up_down_list,dst->id,s_surf_parsing_link_up_down_t);
        if(info.link_down) { // link down
          xbt_dynar_push_as(route->link_list,void*,info.link_down);
        if (lat)
          *lat += surf_network_model->extension.network.get_link_latency(info.link_down);
        }
    }
}

static void model_cluster_finalize(AS_t as) {
  xbt_dynar_free(&(as->link_up_down_list));
  model_none_finalize(as);
}

static int cluster_parse_PU(AS_t rc, sg_routing_edge_t elm) {
  XBT_DEBUG("Load process unit \"%s\"", elm->name);
  xbt_dynar_push_as(rc->index_network_elm,sg_routing_edge_t,elm);
  return xbt_dynar_length(rc->index_network_elm)-1;
}

static int cluster_parse_AS(AS_t rc, sg_routing_edge_t elm) {
  XBT_DEBUG("Load Autonomous system \"%s\"", elm->name);
  xbt_dynar_push_as(rc->index_network_elm,sg_routing_edge_t,elm);
  return xbt_dynar_length(rc->index_network_elm)-1;
}

/* Creation routing model functions */
AS_t model_cluster_create(void)
{
  AS_t result = model_none_create_sized(sizeof(s_as_cluster_t));
  result->get_route_and_latency = cluster_get_route_and_latency;
  result->finalize = model_cluster_finalize;
  result->parse_AS = cluster_parse_AS;
  result->parse_PU = cluster_parse_PU;

  return (AS_t) result;
}
