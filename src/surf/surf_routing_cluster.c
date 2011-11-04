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


	  xbt_dynar_t links_list =
	      xbt_dynar_new(global_routing->size_of_link, NULL);

	  char *cluster_is_fd = xbt_dict_get_or_null(cluster_host_link,rc->name);
	  char *link_src,*link_bb,*link_dst,*link_src_up,*link_dst_down;

	  if(!cluster_is_fd){ //	NOT FULLDUPLEX
		  link_src = xbt_dict_get_or_null(cluster_host_link,src);
		  if( !link_src && (global_routing->get_network_element_type(src) != SURF_NETWORK_ELEMENT_ROUTER) )
			  xbt_die("No link for '%s' found!",src);
		  if(link_src) xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_src, SURF_LINK_LEVEL)); //link_up

		  link_bb = bprintf("%s_backbone",rc->name);
		  xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_bb, SURF_LINK_LEVEL)); //link_bb
		  free(link_bb);

		  link_dst = xbt_dict_get_or_null(cluster_host_link,dst);
		  if( !link_dst && (global_routing->get_network_element_type(dst) != SURF_NETWORK_ELEMENT_ROUTER) )
		  	  xbt_die("No link for '%s' found!",dst);
		  if(link_dst) xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_dst, SURF_LINK_LEVEL)); //link_down
	  }
	  else //	FULLDUPLEX
	  {
		  link_src = xbt_dict_get_or_null(cluster_host_link,src);
		  if( !link_src  && (global_routing->get_network_element_type(src) != SURF_NETWORK_ELEMENT_ROUTER) )
			  xbt_die("No link for '%s' found!",src);
		  link_src_up = bprintf("%s_UP",link_src);
		  if(link_src) xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_src_up, SURF_LINK_LEVEL)); //link_up
		  free(link_src_up);

		  link_bb = bprintf("%s_backbone",rc->name);
		  if(link_bb)  xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_bb, SURF_LINK_LEVEL)); //link_bb
		  free(link_bb);

		  link_dst = xbt_dict_get_or_null(cluster_host_link,dst);
		  if(!link_dst  && (global_routing->get_network_element_type(dst) != SURF_NETWORK_ELEMENT_ROUTER))
			  xbt_die("No link for '%s' found!",dst);
		  link_dst_down = bprintf("%s_DOWN",link_dst);
		  if(link_dst) xbt_dynar_push_as(links_list,void*,xbt_lib_get_or_null(link_lib, link_dst_down, SURF_LINK_LEVEL)); //link_down
		  free(link_dst_down);
	  }

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
