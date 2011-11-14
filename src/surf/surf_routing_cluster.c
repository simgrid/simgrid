/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;

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
static route_t cluster_get_route(AS_t as,
                                          const char *src,
                                          const char *dst) {

	  xbt_dynar_t links_list = xbt_dynar_new(global_routing->size_of_link, NULL);

	  surf_parsing_link_up_down_t info;

	  info = xbt_dict_get_or_null(cluster_host_link,src);
	  if(info) xbt_dynar_push_as(links_list,void*,info->link_up); //link_up

	  if ( ((as_cluster_t)as)->backbone )
	    xbt_dynar_push_as(links_list,void*, ((as_cluster_t)as)->backbone) ;

	  info = xbt_dict_get_or_null(cluster_host_link,dst);
	  if(info) xbt_dynar_push_as(links_list,void*,info->link_down); //link_down

	  route_t new_e_route = NULL;
	  new_e_route = xbt_new0(s_route_t, 1);
	  new_e_route->link_list = links_list;

	  return new_e_route;
}

static void model_cluster_finalize(AS_t as) {
  xbt_dict_free(&cluster_host_link);
  model_none_finalize(as);
}
/* Creation routing model functions */
AS_t model_cluster_create(void)
{
  AS_t result = model_none_create_sized(sizeof(s_as_cluster_t));
  result->get_route = cluster_get_route;
  result->get_latency = generic_get_link_latency;
  result->finalize = model_cluster_finalize;

  return (AS_t) result;
}

void surf_routing_cluster_add_link(const char* host_id,surf_parsing_link_up_down_t info) {
  if(!cluster_host_link)
    cluster_host_link = xbt_dict_new();

 xbt_dict_set(cluster_host_link,host_id,info,xbt_free);
}

void surf_routing_cluster_add_backbone(AS_t as, void* bb) {
  ((as_cluster_t)as)->backbone = bb;
}
