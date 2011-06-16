/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern routing_component_t current_routing;
extern model_type_t current_routing_model;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

#define TO_FLOYD_COST(i,j) (routing->cost_table)[(i)+(j)*table_size]
#define TO_FLOYD_PRED(i,j) (routing->predecessor_table)[(i)+(j)*table_size]
#define TO_FLOYD_LINK(i,j) (routing->link_table)[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_routing_component_t generic_routing;
  /* vars for calculate the floyd algorith. */
  int *predecessor_table;
  double *cost_table;
  route_extended_t *link_table; /* char* -> int* */
} s_routing_component_floyd_t, *routing_component_floyd_t;

static route_extended_t floyd_get_route(routing_component_t rc,
                                        const char *src, const char *dst);

/* Business methods */
static xbt_dynar_t floyd_get_onelink_routes(routing_component_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  //size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
  xbt_dict_cursor_t c1 = NULL, c2 = NULL;
  char *k1, *d1, *k2, *d2;
  xbt_dict_foreach(routing->generic_routing.to_index, c1, k1, d1) {
    xbt_dict_foreach(routing->generic_routing.to_index, c2, k2, d2) {
      route_extended_t route = floyd_get_route(rc, k1, k2);
      if (route) {
        if (xbt_dynar_length(route->generic_route.link_list) == 1) {
          void *link =
              *(void **) xbt_dynar_get_ptr(route->generic_route.link_list,
                                           0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (routing->generic_routing.hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_strdup(k1);
            onelink->dst = xbt_strdup(k2);
          } else if (routing->generic_routing.hierarchy ==
                     SURF_ROUTING_RECURSIVE) {
            onelink->src = xbt_strdup(route->src_gateway);
            onelink->dst = xbt_strdup(route->dst_gateway);
          }
          xbt_dynar_push(ret, &onelink);
        }
      }
    }
  }
  return ret;
}

static route_extended_t floyd_get_route(routing_component_t rc,
                                        const char *src, const char *dst)
{
  xbt_assert(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  generic_src_dst_check(rc, src, dst);
  int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, src);
  int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, dst);
  xbt_assert(src_id
              && dst_id,
              "Ask for route \"from\"(%s)  or \"to\"(%s) no found in the local table",
              src, dst);

  /* create a result route */
  route_extended_t new_e_route = xbt_new0(s_route_extended_t, 1);
  new_e_route->generic_route.link_list =
      xbt_dynar_new(global_routing->size_of_link, NULL);
  new_e_route->src_gateway = NULL;
  new_e_route->dst_gateway = NULL;

  int first = 1;
  int pred = *dst_id;
  int prev_pred = 0;
  char *gw_src = NULL, *gw_dst = NULL, *prev_gw_src, *first_gw = NULL;
  unsigned int cpt;
  void *link;
  xbt_dynar_t links;

  do {
    prev_pred = pred;
    pred = TO_FLOYD_PRED(*src_id, pred);
    if (pred == -1)             /* if no pred in route -> no route to host */
      break;
    xbt_assert(TO_FLOYD_LINK(pred, prev_pred),
                "Invalid link for the route between \"%s\" or \"%s\"", src,
                dst);

    prev_gw_src = gw_src;

    route_extended_t e_route = TO_FLOYD_LINK(pred, prev_pred);
    gw_src = e_route->src_gateway;
    gw_dst = e_route->dst_gateway;

    if (first)
      first_gw = gw_dst;

    if (rc->hierarchy == SURF_ROUTING_RECURSIVE && !first
        && strcmp(gw_dst, prev_gw_src)) {
      xbt_dynar_t e_route_as_to_as =
          (*(global_routing->get_route)) (gw_dst, prev_gw_src);
      xbt_assert(e_route_as_to_as, "no route between \"%s\" and \"%s\"",
                  gw_dst, prev_gw_src);
      links = e_route_as_to_as;
      int pos = 0;
      xbt_dynar_foreach(links, cpt, link) {
        xbt_dynar_insert_at(new_e_route->generic_route.link_list, pos,
                            &link);
        pos++;
      }
    }

    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(new_e_route->generic_route.link_list, &link);
    }
    first = 0;

  } while (pred != *src_id);
  xbt_assert(pred != -1, "no route from host %d to %d (\"%s\" to \"%s\")",
              *src_id, *dst_id, src, dst);

  if (rc->hierarchy == SURF_ROUTING_RECURSIVE) {
    new_e_route->src_gateway = xbt_strdup(gw_src);
    new_e_route->dst_gateway = xbt_strdup(first_gw);
  }

  return new_e_route;
}

static void floyd_finalize(routing_component_t rc)
{
  routing_component_floyd_t routing = (routing_component_floyd_t) rc;
  int i, j;
  size_t table_size;
  if (routing) {
    table_size = xbt_dict_length(routing->generic_routing.to_index);
    /* Delete link_table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++)
        generic_free_extended_route(TO_FLOYD_LINK(i, j));
    xbt_free(routing->link_table);
    /* Delete bypass dict */
    xbt_dict_free(&routing->generic_routing.bypassRoutes);
    /* Delete index dict */
    xbt_dict_free(&(routing->generic_routing.to_index));
    /* Delete dictionary index dict, predecessor and links table */
    xbt_free(routing->predecessor_table);
    /* Delete structure */
    xbt_free(rc);
  }
}

void *model_floyd_create(void)
{
  routing_component_floyd_t new_component =
      xbt_new0(s_routing_component_floyd_t, 1);
  new_component->generic_routing.set_processing_unit =
      generic_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      generic_set_autonomous_system;
  new_component->generic_routing.set_route = model_floyd_set_route;
  new_component->generic_routing.set_ASroute = model_floyd_set_route;
  new_component->generic_routing.set_bypassroute = generic_set_bypassroute;
  new_component->generic_routing.get_route = floyd_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_onelink_routes =
      floyd_get_onelink_routes;
  new_component->generic_routing.get_bypass_route =
      generic_get_bypassroute;
  new_component->generic_routing.finalize = floyd_finalize;
  new_component->generic_routing.to_index = xbt_dict_new();
  new_component->generic_routing.bypassRoutes = xbt_dict_new();
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  return new_component;
}

void model_floyd_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

void model_floyd_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

void model_floyd_end(void)
{

	routing_component_floyd_t routing =
	  ((routing_component_floyd_t) current_routing);

	unsigned int i, j, a, b, c;

	/* set the size of table routing */
	size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

	if(!routing->link_table)
	{
		/* Create Cost, Predecessor and Link tables */
		routing->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
		routing->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
		routing->link_table = xbt_new0(route_extended_t, table_size * table_size);    /* actual link between src and dst */

		/* Initialize costs and predecessors */
		for (i = 0; i < table_size; i++)
		for (j = 0; j < table_size; j++) {
		  TO_FLOYD_COST(i, j) = DBL_MAX;
		  TO_FLOYD_PRED(i, j) = -1;
		  TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
		}
	}

	/* Add the loopback if needed */
	if (current_routing->hierarchy == SURF_ROUTING_BASE) {
		for (i = 0; i < table_size; i++) {
		  route_extended_t e_route = TO_FLOYD_LINK(i, i);
		  if (!e_route) {
			e_route = xbt_new0(s_route_extended_t, 1);
			e_route->src_gateway = NULL;
			e_route->dst_gateway = NULL;
			e_route->generic_route.link_list =
				xbt_dynar_new(global_routing->size_of_link, NULL);
			xbt_dynar_push(e_route->generic_route.link_list,
						   &global_routing->loopback);
			TO_FLOYD_LINK(i, i) = e_route;
			TO_FLOYD_PRED(i, i) = i;
			TO_FLOYD_COST(i, i) = 1;
		  }
		}
	}
	/* Calculate path costs */
	for (c = 0; c < table_size; c++) {
		for (a = 0; a < table_size; a++) {
		  for (b = 0; b < table_size; b++) {
			if (TO_FLOYD_COST(a, c) < DBL_MAX && TO_FLOYD_COST(c, b) < DBL_MAX) {
			  if (TO_FLOYD_COST(a, b) == DBL_MAX ||
				  (TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b) <
				   TO_FLOYD_COST(a, b))) {
				TO_FLOYD_COST(a, b) =
					TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b);
				TO_FLOYD_PRED(a, b) = TO_FLOYD_PRED(c, b);
			  }
			}
		  }
		}
	}
}

void model_floyd_set_route(routing_component_t rc, const char *src,
        const char *dst, name_route_extended_t route)
{
	routing_component_floyd_t routing = (routing_component_floyd_t) rc;

	/* set the size of table routing */
	size_t table_size = xbt_dict_length(rc->to_index);
	int *src_id, *dst_id;
	int i,j;

	src_id = xbt_dict_get_or_null(rc->to_index, src);
	dst_id = xbt_dict_get_or_null(rc->to_index, dst);

	xbt_assert(src_id, "Network elements %s not found", src);
	xbt_assert(dst_id, "Network elements %s not found", dst);

	if(!routing->link_table)
	{
		/* Create Cost, Predecessor and Link tables */
		routing->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
		routing->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
		routing->link_table = xbt_new0(route_extended_t, table_size * table_size);    /* actual link between src and dst */

		/* Initialize costs and predecessors */
		for (i = 0; i < table_size; i++)
		for (j = 0; j < table_size; j++) {
		  TO_FLOYD_COST(i, j) = DBL_MAX;
		  TO_FLOYD_PRED(i, j) = -1;
		  TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
		}
	}

	if(TO_FLOYD_LINK(*src_id, *dst_id))
	{
		if(!route->dst_gateway && !route->src_gateway)
			XBT_DEBUG("See Route from \"%s\" to \"%s\"", src, dst);
		else
			XBT_DEBUG("See ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
				 route->src_gateway, dst, route->dst_gateway);
		char * link_name;
		unsigned int cpt;
		xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
		xbt_dynar_foreach(route->generic_route.link_list,cpt,link_name)
		{
			void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
			xbt_assert(link,"Link : '%s' doesn't exists.",link_name);
			xbt_dynar_push(link_route_to_test,&link);
		}
		xbt_assert(!xbt_dynar_compare(
			  (void*)TO_FLOYD_LINK(*src_id, *dst_id)->generic_route.link_list,
			  (void*)link_route_to_test,
			  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
			  "The route between \"%s\" and \"%s\" already exists", src,dst);
	}
	else
	{
		if(!route->dst_gateway && !route->src_gateway)
		  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
		else{
			XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
				 route->src_gateway, dst, route->dst_gateway);
			if(global_routing->get_network_element_type((const char*)route->dst_gateway) == SURF_NETWORK_ELEMENT_NULL)
				xbt_die("The dst_gateway '%s' does not exist!",route->dst_gateway);
			if(global_routing->get_network_element_type((const char*)route->src_gateway) == SURF_NETWORK_ELEMENT_NULL)
				xbt_die("The src_gateway '%s' does not exist!",route->src_gateway);
		}
	    TO_FLOYD_LINK(*src_id, *dst_id) =
	    		generic_new_extended_route(rc->hierarchy, route, 1);
	    TO_FLOYD_PRED(*src_id, *dst_id) = *src_id;
	    TO_FLOYD_COST(*src_id, *dst_id) =
	    		((TO_FLOYD_LINK(*src_id, *dst_id))->generic_route.link_list)->used;   /* count of links, old model assume 1 */
	}

	if( A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
		|| A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES )
	{
		if(TO_FLOYD_LINK(*dst_id, *src_id))
		{
			if(!route->dst_gateway && !route->src_gateway)
			  XBT_DEBUG("See Route from \"%s\" to \"%s\"", dst, src);
			else
			  XBT_DEBUG("See ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
					 route->src_gateway, src, route->dst_gateway);
			char * link_name;
			unsigned int i;
			xbt_dynar_t link_route_to_test = xbt_dynar_new(global_routing->size_of_link, NULL);
			for(i=xbt_dynar_length(route->generic_route.link_list) ;i>0 ;i--)
			{
				link_name = xbt_dynar_get_as(route->generic_route.link_list,i-1,void *);
				void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
				xbt_assert(link,"Link : '%s' doesn't exists.",link_name);
				xbt_dynar_push(link_route_to_test,&link);
			}
			xbt_assert(!xbt_dynar_compare(
				  (void*)TO_FLOYD_LINK(*dst_id, *src_id)->generic_route.link_list,
			      (void*)link_route_to_test,
				  (int_f_cpvoid_cpvoid_t) surf_pointer_resource_cmp),
				  "The route between \"%s\" and \"%s\" already exists", src,dst);
		}
		else
		{
			if(route->dst_gateway && route->src_gateway)
			{
                          char *gw_src = xbt_strdup(route->src_gateway);
                          char *gw_dst = xbt_strdup(route->dst_gateway);
                          route->src_gateway = gw_dst;
                          route->dst_gateway = gw_src;
			}

			if(!route->dst_gateway && !route->src_gateway)
			  XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
			else
			  XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
					 route->src_gateway, src, route->dst_gateway);

		    TO_FLOYD_LINK(*dst_id, *src_id) =
		    		generic_new_extended_route(rc->hierarchy, route, 0);
		    TO_FLOYD_PRED(*dst_id, *src_id) = *dst_id;
		    TO_FLOYD_COST(*dst_id, *src_id) =
		    		((TO_FLOYD_LINK(*dst_id, *src_id))->generic_route.link_list)->used;   /* count of links, old model assume 1 */
		}
	}
}
