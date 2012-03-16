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

typedef struct {
  s_as_t generic_routing;
  void *backbone;
} s_as_cluster_t, *as_cluster_t;


static xbt_dict_t cluster_host_link = NULL;

/* Business methods */
static void cluster_get_route_and_latency(AS_t as,
    network_element_t src, network_element_t dst,
                                          route_t route, double *lat) {

	  surf_parsing_link_up_down_t info;

	  info = xbt_dict_get_or_null(cluster_host_link,src->name);
	  if(info) { // link up
	    xbt_dynar_push_as(route->link_list,void*,info->link_up);
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(info->link_up);
	  }

	  if ( ((as_cluster_t)as)->backbone ) {
	    xbt_dynar_push_as(route->link_list,void*, ((as_cluster_t)as)->backbone) ;
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(((as_cluster_t)as)->backbone);
	  }

	  info = xbt_dict_get_or_null(cluster_host_link,dst->name);
	  if(info) { // link down
	    xbt_dynar_push_as(route->link_list,void*,info->link_down);
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(info->link_down);
	  }
}

static void model_cluster_finalize(AS_t as) {
  xbt_dict_free(&cluster_host_link);
  model_none_finalize(as);
}
/* Creation routing model functions */
AS_t model_cluster_create(void)
{
  AS_t result = model_none_create_sized(sizeof(s_as_cluster_t));
  result->get_route_and_latency = cluster_get_route_and_latency;
  result->finalize = model_cluster_finalize;

  return (AS_t) result;
}

void surf_routing_cluster_add_link(const char* host_id,surf_parsing_link_up_down_t info) {
  if(!cluster_host_link)
    cluster_host_link = xbt_dict_new_homogeneous(xbt_free);

 xbt_dict_set(cluster_host_link,host_id,info,NULL);
}

void surf_routing_cluster_add_backbone(AS_t as, void* bb) {
  ((as_cluster_t)as)->backbone = bb;
}
