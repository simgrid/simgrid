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

static xbt_dict_t cluster_host_link = NULL; /* for tag cluster */

/* Business methods */
static route_extended_t cluster_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst) {

	  xbt_dynar_t links_list = xbt_dynar_new(global_routing->size_of_link, NULL);

	  surf_parsing_link_up_down_t info;

	  info = xbt_dict_get_or_null(cluster_host_link,src);
	  if(info) xbt_dynar_push_as(links_list,void*,info->link_up); //link_up

	  info = xbt_dict_get_or_null(cluster_host_link,rc->name);
	  if(info)  xbt_dynar_push_as(links_list,void*,info->link_up); //link_bb

	  info = xbt_dict_get_or_null(cluster_host_link,dst);
	  if(info) xbt_dynar_push_as(links_list,void*,info->link_down); //link_down

	  route_extended_t new_e_route = NULL;
	  new_e_route = xbt_new0(s_route_extended_t, 1);
	  new_e_route->generic_route.link_list = links_list;

	  return new_e_route;
}

/* Creation routing model functions */
routing_component_t model_cluster_create(void)
{
  routing_component_t new_component = model_none_create();
  new_component->get_route = cluster_get_route;

  return (routing_component_t) new_component;
}

void surf_routing_cluster_add_link(const char* host_id,surf_parsing_link_up_down_t info) {
  if(!cluster_host_link)
    cluster_host_link = xbt_dict_new();

 xbt_dict_set(cluster_host_link,host_id,info,xbt_free);
}
