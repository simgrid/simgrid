/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern routing_component_t current_routing;
extern model_type_t current_routing_model;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_dijkstra, surf, "Routing part of surf");

typedef struct {
  s_routing_component_t generic_routing;
  xbt_graph_t route_graph;      /* xbt_graph */
  xbt_dict_t graph_node_map;    /* map */
  xbt_dict_t route_cache;       /* use in cache mode */
  int cached;
} s_routing_component_dijkstra_t, *routing_component_dijkstra_t;


typedef struct graph_node_data {
  int id;
  int graph_id;                 /* used for caching internal graph id's */
} s_graph_node_data_t, *graph_node_data_t;

typedef struct graph_node_map_element {
  xbt_node_t node;
} s_graph_node_map_element_t, *graph_node_map_element_t;

typedef struct route_cache_element {
  int *pred_arr;
  int size;
} s_route_cache_element_t, *route_cache_element_t;

/* Free functions */

static void route_cache_elem_free(void *e)
{
  route_cache_element_t elm = (route_cache_element_t) e;
  if (elm) {
    xbt_free(elm->pred_arr);
    xbt_free(elm);
  }
}

static void graph_node_map_elem_free(void *e)
{
  graph_node_map_element_t elm = (graph_node_map_element_t) e;
  if (elm) {
    xbt_free(elm);
  }
}

static void graph_edge_data_free(void *e)
{
  route_extended_t e_route = (route_extended_t) e;
  if (e_route) {
    xbt_dynar_free(&(e_route->generic_route.link_list));
    if (e_route->src_gateway)
      xbt_free(e_route->src_gateway);
    if (e_route->dst_gateway)
      xbt_free(e_route->dst_gateway);
    xbt_free(e_route);
  }
}

/* Utility functions */

static xbt_node_t route_graph_new_node(routing_component_dijkstra_t rc,
                                       int id, int graph_id)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  xbt_node_t node = NULL;
  graph_node_data_t data = NULL;
  graph_node_map_element_t elm = NULL;

  data = xbt_new0(struct graph_node_data, 1);
  data->id = id;
  data->graph_id = graph_id;
  node = xbt_graph_new_node(routing->route_graph, data);

  elm = xbt_new0(struct graph_node_map_element, 1);
  elm->node = node;
  xbt_dict_set_ext(routing->graph_node_map, (char *) (&id), sizeof(int),
                   (xbt_set_elm_t) elm, &graph_node_map_elem_free);

  return node;
}

static graph_node_map_element_t
graph_node_map_search(routing_component_dijkstra_t rc, int id)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  graph_node_map_element_t elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&id),
                               sizeof(int));
  return elm;
}

/* Parsing */

static void route_new_dijkstra(routing_component_dijkstra_t rc, int src_id,
                               int dst_id, route_extended_t e_route)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
  XBT_DEBUG("Load Route from \"%d\" to \"%d\"", src_id, dst_id);
  xbt_node_t src = NULL;
  xbt_node_t dst = NULL;

  graph_node_map_element_t src_elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&src_id),
                               sizeof(int));
  graph_node_map_element_t dst_elm = (graph_node_map_element_t)
      xbt_dict_get_or_null_ext(routing->graph_node_map,
                               (char *) (&dst_id),
                               sizeof(int));


  if (src_elm)
    src = src_elm->node;

  if (dst_elm)
    dst = dst_elm->node;

  /* add nodes if they don't exist in the graph */
  if (src_id == dst_id && src == NULL && dst == NULL) {
    src = route_graph_new_node(rc, src_id, -1);
    dst = src;
  } else {
    if (src == NULL) {
      src = route_graph_new_node(rc, src_id, -1);
    }
    if (dst == NULL) {
      dst = route_graph_new_node(rc, dst_id, -1);
    }
  }

  /* add link as edge to graph */
  xbt_graph_new_edge(routing->route_graph, src, dst, e_route);
}

static void add_loopback_dijkstra(routing_component_dijkstra_t rc)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

  xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);

  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_foreach(nodes, cursor2, node) {
    xbt_dynar_t out_edges = xbt_graph_node_get_outedges(node);
    xbt_edge_t edge = NULL;
    unsigned int cursor;

    int found = 0;
    xbt_dynar_foreach(out_edges, cursor, edge) {
      xbt_node_t other_node = xbt_graph_edge_get_target(edge);
      if (other_node == node) {
        found = 1;
        break;
      }
    }

    if (!found) {
      route_extended_t e_route = xbt_new0(s_route_extended_t, 1);
      e_route->src_gateway = NULL;
      e_route->dst_gateway = NULL;
      e_route->generic_route.link_list =
          xbt_dynar_new(global_routing->size_of_link, NULL);
      xbt_dynar_push(e_route->generic_route.link_list,
                     &global_routing->loopback);
      xbt_graph_new_edge(routing->route_graph, node, node, e_route);
    }
  }
}

/* Business methods */
static xbt_dynar_t dijkstra_get_onelink_routes(routing_component_t rc)
{
  xbt_die("\"dijkstra_get_onelink_routes\" function not implemented yet");
}

static route_extended_t dijkstra_get_route(routing_component_t rc,
                                           const char *src,
                                           const char *dst)
{
  xbt_assert(rc && src
              && dst,
              "Invalid params for \"get_route\" function at AS \"%s\"",
              rc->name);

  /* set utils vars */
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

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

  int *pred_arr = NULL;
  int src_node_id = 0;
  int dst_node_id = 0;
  int *nodeid = NULL;
  int v;
  route_extended_t e_route;
  int size = 0;
  unsigned int cpt;
  void *link;
  xbt_dynar_t links = NULL;
  route_cache_element_t elm = NULL;
  xbt_dynar_t nodes = xbt_graph_get_nodes(routing->route_graph);

  /* Use the graph_node id mapping set to quickly find the nodes */
  graph_node_map_element_t src_elm =
      graph_node_map_search(routing, *src_id);
  graph_node_map_element_t dst_elm =
      graph_node_map_search(routing, *dst_id);
  xbt_assert(src_elm != NULL
              && dst_elm != NULL, "src %d or dst %d does not exist",
              *src_id, *dst_id);
  src_node_id = ((graph_node_data_t)
                 xbt_graph_node_get_data(src_elm->node))->graph_id;
  dst_node_id = ((graph_node_data_t)
                 xbt_graph_node_get_data(dst_elm->node))->graph_id;

  /* if the src and dst are the same *//* fixed, missing in the previous version */
  if (src_node_id == dst_node_id) {

    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(routing->route_graph, node_s_v, node_e_v);

    xbt_assert(edge != NULL, "no route between host %d and %d", *src_id,
                *dst_id);

    e_route = (route_extended_t) xbt_graph_edge_get_data(edge);

    links = e_route->generic_route.link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(new_e_route->generic_route.link_list, &link);
    }

    return new_e_route;
  }

  if (routing->cached) {
    /*check if there is a cached predecessor list avail */
    elm = (route_cache_element_t)
        xbt_dict_get_or_null_ext(routing->route_cache, (char *) (&src_id),
                                 sizeof(int));
  }

  if (elm) {                    /* cached mode and cache hit */
    pred_arr = elm->pred_arr;
  } else {                      /* not cached mode or cache miss */
    double *cost_arr = NULL;
    xbt_heap_t pqueue = NULL;
    int i = 0;

    int nr_nodes = xbt_dynar_length(nodes);
    cost_arr = xbt_new0(double, nr_nodes);      /* link cost from src to other hosts */
    pred_arr = xbt_new0(int, nr_nodes); /* predecessors in path from src */
    pqueue = xbt_heap_new(nr_nodes, xbt_free);

    /* initialize */
    cost_arr[src_node_id] = 0.0;

    for (i = 0; i < nr_nodes; i++) {
      if (i != src_node_id) {
        cost_arr[i] = DBL_MAX;
      }

      pred_arr[i] = 0;

      /* initialize priority queue */
      nodeid = xbt_new0(int, 1);
      *nodeid = i;
      xbt_heap_push(pqueue, nodeid, cost_arr[i]);

    }

    /* apply dijkstra using the indexes from the graph's node array */
    while (xbt_heap_size(pqueue) > 0) {
      int *v_id = xbt_heap_pop(pqueue);
      xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
      xbt_dynar_t out_edges = xbt_graph_node_get_outedges(v_node);
      xbt_edge_t edge = NULL;
      unsigned int cursor;

      xbt_dynar_foreach(out_edges, cursor, edge) {
        xbt_node_t u_node = xbt_graph_edge_get_target(edge);
        graph_node_data_t data = xbt_graph_node_get_data(u_node);
        int u_id = data->graph_id;
        route_extended_t tmp_e_route =
            (route_extended_t) xbt_graph_edge_get_data(edge);
        int cost_v_u = (tmp_e_route->generic_route.link_list)->used;    /* count of links, old model assume 1 */

        if (cost_v_u + cost_arr[*v_id] < cost_arr[u_id]) {
          pred_arr[u_id] = *v_id;
          cost_arr[u_id] = cost_v_u + cost_arr[*v_id];
          nodeid = xbt_new0(int, 1);
          *nodeid = u_id;
          xbt_heap_push(pqueue, nodeid, cost_arr[u_id]);
        }
      }

      /* free item popped from pqueue */
      xbt_free(v_id);
    }

    xbt_free(cost_arr);
    xbt_heap_free(pqueue);
  }

  /* compose route path with links */
  char *gw_src = NULL, *gw_dst =
      NULL, *prev_gw_src, *prev_gw_dst, *first_gw = NULL;

  for (v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v =
        xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(routing->route_graph, node_pred_v, node_v);

    xbt_assert(edge != NULL, "no route between host %d and %d", *src_id,
                *dst_id);

    prev_gw_src = gw_src;
    prev_gw_dst = gw_dst;

    e_route = (route_extended_t) xbt_graph_edge_get_data(edge);
    gw_src = e_route->src_gateway;
    gw_dst = e_route->dst_gateway;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (rc->hierarchy == SURF_ROUTING_RECURSIVE && v != dst_node_id
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
    size++;
  }

  if (rc->hierarchy == SURF_ROUTING_RECURSIVE) {
    new_e_route->src_gateway = xbt_strdup(gw_src);
    new_e_route->dst_gateway = xbt_strdup(first_gw);
  }

  if (routing->cached && elm == NULL) {
    /* add to predecessor list of the current src-host to cache */
    elm = xbt_new0(struct route_cache_element, 1);
    elm->pred_arr = pred_arr;
    elm->size = size;
    xbt_dict_set_ext(routing->route_cache, (char *) (&src_id), sizeof(int),
                     (xbt_set_elm_t) elm, &route_cache_elem_free);
  }

  if (!routing->cached)
    xbt_free(pred_arr);

  return new_e_route;
}

static void dijkstra_finalize(routing_component_t rc)
{
  routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;

  if (routing) {
    xbt_graph_free_graph(routing->route_graph, &xbt_free,
                         &graph_edge_data_free, &xbt_free);
    xbt_dict_free(&routing->graph_node_map);
    if (routing->cached)
      xbt_dict_free(&routing->route_cache);
    /* Delete bypass dict */
    xbt_dict_free(&routing->generic_routing.bypassRoutes);
    /* Delete index dict */
    xbt_dict_free(&(routing->generic_routing.to_index));
    /* Delete structure */
    xbt_free(routing);
  }
}

/* Creation routing model functions */

void *model_dijkstra_both_create(int cached)
{
  routing_component_dijkstra_t new_component =
      xbt_new0(s_routing_component_dijkstra_t, 1);
  new_component->generic_routing.set_processing_unit =
      generic_set_processing_unit;
  new_component->generic_routing.set_autonomous_system =
      generic_set_autonomous_system;
  new_component->generic_routing.set_route = model_dijkstra_both_set_route;
  new_component->generic_routing.set_ASroute = model_dijkstra_both_set_route;
  new_component->generic_routing.set_bypassroute = generic_set_bypassroute;
  new_component->generic_routing.get_route = dijkstra_get_route;
  new_component->generic_routing.get_latency = generic_get_link_latency;
  new_component->generic_routing.get_onelink_routes =
      dijkstra_get_onelink_routes;
  new_component->generic_routing.get_bypass_route =
      generic_get_bypassroute;
  new_component->generic_routing.finalize = dijkstra_finalize;
  new_component->cached = cached;
  new_component->generic_routing.to_index = xbt_dict_new();
  new_component->generic_routing.bypassRoutes = xbt_dict_new();
  new_component->generic_routing.get_network_element_type = get_network_element_type;
  return new_component;
}

void *model_dijkstra_create(void)
{
  return model_dijkstra_both_create(0);
}

void *model_dijkstracache_create(void)
{
  return model_dijkstra_both_create(1);
}

void model_dijkstra_both_load(void)
{
  /* use "surfxml_add_callback" to add a parse function call */
}

void model_dijkstra_both_unload(void)
{
  /* use "surfxml_del_callback" to remove a parse function call */
}

void model_dijkstra_both_end(void)
{
  routing_component_dijkstra_t routing =
      (routing_component_dijkstra_t) current_routing;

  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_t nodes = NULL;

  /* Create the topology graph */
  if(!routing->route_graph)
  routing->route_graph = xbt_graph_new_graph(1, NULL);
  if(!routing->graph_node_map)
  routing->graph_node_map = xbt_dict_new();

  if (routing->cached && !routing->route_cache)
  routing->route_cache = xbt_dict_new();

  /* Add the loopback if needed */
  if (current_routing->hierarchy == SURF_ROUTING_BASE)
    add_loopback_dijkstra(routing);

  /* initialize graph indexes in nodes after graph has been built */
  nodes = xbt_graph_get_nodes(routing->route_graph);

  xbt_dynar_foreach(nodes, cursor2, node) {
    graph_node_data_t data = xbt_graph_node_get_data(node);
    data->graph_id = cursor2;
  }

}
void model_dijkstra_both_set_route (routing_component_t rc, const char *src,
                     const char *dst, name_route_extended_t route)
{
	routing_component_dijkstra_t routing = (routing_component_dijkstra_t) rc;
	int *src_id, *dst_id;
	src_id = xbt_dict_get_or_null(rc->to_index, src);
	dst_id = xbt_dict_get_or_null(rc->to_index, dst);

    /* Create the topology graph */
	if(!routing->route_graph)
	routing->route_graph = xbt_graph_new_graph(1, NULL);
	if(!routing->graph_node_map)
	routing->graph_node_map = xbt_dict_new();

	if (routing->cached && !routing->route_cache)
	routing->route_cache = xbt_dict_new();

	if( A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
		|| A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES )
		xbt_die("Route symmetrical not supported on model dijkstra");

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

	route_extended_t e_route =
		generic_new_extended_route(current_routing->hierarchy, route, 1);
	route_new_dijkstra(routing, *src_id, *dst_id, e_route);
}
