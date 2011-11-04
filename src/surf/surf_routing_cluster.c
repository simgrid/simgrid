/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern routing_component_t current_routing;
extern model_type_t current_routing_model;
extern xbt_dynar_t link_list;
extern xbt_dict_t cluster_host_link;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  xbt_dict_t dict_processing_units;
  xbt_dict_t dict_autonomous_systems;
} s_routing_component_cluster_t, *routing_component_cluster_t;

/* Parse routing model functions */

static route_extended_t cluster_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst);

/* Business methods */
static route_extended_t cluster_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst)
{
	  xbt_assert(rc && src
	              && dst,
	              "Invalid params for \"get_route\" function at AS \"%s\"",
	              rc->name);


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
void *model_cluster_create(void)
{
  routing_component_cluster_t new_component = model_rulebased_create();
  new_component->generic_routing.get_route = cluster_get_route;

  return new_component;
}
