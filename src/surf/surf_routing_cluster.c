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

static void model_cluster_set_processing_unit(routing_component_t rc,
                                                const char *name)
{
  routing_component_cluster_t routing =
      (routing_component_cluster_t) rc;
  xbt_dict_set(routing->dict_processing_units, name, (void *) (-1), NULL);
}

static void model_cluster_set_autonomous_system(routing_component_t rc,
                                                  const char *name)
{
  routing_component_cluster_t routing =
      (routing_component_cluster_t) rc;
  xbt_dict_set(routing->dict_autonomous_systems, name, (void *) (-1),
               NULL);
}

static void model_cluster_set_bypassroute(routing_component_t rc,
                                            const char *src,
                                            const char *dst,
                                            route_extended_t e_route)
{
  xbt_die("bypass routing not supported for Route-Based model");
}

static void model_cluster_set_route(routing_component_t rc,
                                      const char *src, const char *dst,
                                      name_route_extended_t route)
{
	xbt_die("routing not supported for Route-Based model");
}

static void model_cluster_set_ASroute(routing_component_t rc,
                                        const char *src, const char *dst,
                                        name_route_extended_t route)
{
	xbt_die("AS routing not supported for Route-Based model");
}

#define BUFFER_SIZE 4096        /* result buffer size */
#define OVECCOUNT 30            /* should be a multiple of 3 */

static route_extended_t cluster_get_route(routing_component_t rc,
                                            const char *src,
                                            const char *dst);
static xbt_dynar_t cluster_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new (sizeof(onelink_t), xbt_free);

  //We have already bypass cluster routes with network NS3
  if(!strcmp(surf_network_model->name,"network NS3"))
	return ret;

  routing_component_cluster_t routing = (routing_component_cluster_t)rc;

  xbt_dict_cursor_t c1 = NULL;
  char *k1, *d1;

  //find router
  char *router = NULL;
  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    if (rc->get_network_element_type(k1) == SURF_NETWORK_ELEMENT_ROUTER){
      router = k1;
    }
  }

  if (!router){
    xbt_die ("cluster_get_onelink_routes works only if the AS is a cluster, sorry.");
  }

  xbt_dict_foreach(routing->dict_processing_units, c1, k1, d1) {
    route_extended_t route = cluster_get_route (rc, router, k1);

    int number_of_links = xbt_dynar_length(route->generic_route.link_list);

    if(number_of_links == 1) {
		//loopback
    }
    else{
		if (number_of_links != 2) {
		  xbt_die ("cluster_get_onelink_routes works only if the AS is a cluster, sorry.");
		}

		void *link_ptr;
		xbt_dynar_get_cpy (route->generic_route.link_list, 1, &link_ptr);
		onelink_t onelink = xbt_new0 (s_onelink_t, 1);
		onelink->src = xbt_strdup (k1);
		onelink->dst = xbt_strdup (router);
		onelink->link_ptr = link_ptr;
		xbt_dynar_push (ret, &onelink);
    }
  }
  return ret;
}

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

static route_extended_t cluster_get_bypass_route(routing_component_t rc,
                                                   const char *src,
                                                   const char *dst)
{
  return NULL;
}

static void cluster_finalize(routing_component_t rc)
{
  routing_component_cluster_t routing =
      (routing_component_cluster_t) rc;
  if (routing) {
    xbt_dict_free(&routing->dict_processing_units);
    xbt_dict_free(&routing->dict_autonomous_systems);
    /* Delete structure */
    xbt_free(routing);
  }
}

/* Creation routing model functions */
void *model_cluster_create(void)
{
  routing_component_cluster_t new_component =
      xbt_new0(s_routing_component_cluster_t, 1);
  new_component->generic_routing.set_processing_unit =
      model_cluster_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      model_cluster_set_autonomous_system;
  new_component->generic_routing.set_route = model_cluster_set_route;
  new_component->generic_routing.set_ASroute = model_cluster_set_ASroute;
  new_component->generic_routing.set_bypassroute = model_cluster_set_bypassroute;
  new_component->generic_routing.get_onelink_routes = cluster_get_onelink_routes;
  new_component->generic_routing.get_route = cluster_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_bypass_route = cluster_get_bypass_route;
  new_component->generic_routing.finalize = cluster_finalize;
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  /* initialization of internal structures */
  new_component->dict_processing_units = xbt_dict_new();
  new_component->dict_autonomous_systems = xbt_dict_new();

  return new_component;
}

void model_cluster_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

void model_cluster_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

void model_cluster_end(void)
{
}
