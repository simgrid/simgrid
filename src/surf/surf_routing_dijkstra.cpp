/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_dijkstra.hpp"
#include "surf_routing_private.h"
#include "network.hpp"

/* Global vars */
extern routing_platf_t routing_platf;

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_dijkstra, surf, "Routing part of surf -- dijkstra routing logic");
}

AS_t model_dijkstra_create(void){
  return new AsDijkstra(0);
}

AS_t model_dijkstracache_create(void){
  return new AsDijkstra(1);
}

void model_dijkstra_both_end(AS_t as)
{
  delete as;
}

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
  xbt_free(elm);
}

static void graph_edge_data_free(void *e) // FIXME: useless code duplication
{
  sg_platf_route_cbarg_t e_route = (sg_platf_route_cbarg_t) e;
  if (e_route) {
    xbt_dynar_free(&(e_route->link_list));
    xbt_free(e_route);
  }
}

/* Utility functions */

xbt_node_t AsDijkstra::routeGraphNewNode(int id, int graph_id)
{
  xbt_node_t node = NULL;
  graph_node_data_t data = NULL;
  graph_node_map_element_t elm = NULL;

  data = xbt_new0(struct graph_node_data, 1);
  data->id = id;
  data->graph_id = graph_id;
  node = xbt_graph_new_node(p_routeGraph, data);

  elm = xbt_new0(struct graph_node_map_element, 1);
  elm->node = node;
  xbt_dict_set_ext(p_graphNodeMap, (char *) (&id), sizeof(int),
      (xbt_set_elm_t) elm, NULL);

  return node;
}

graph_node_map_element_t AsDijkstra::nodeMapSearch(int id)
{
  graph_node_map_element_t elm = (graph_node_map_element_t)
          xbt_dict_get_or_null_ext(p_graphNodeMap,
              (char *) (&id),
              sizeof(int));
  return elm;
}

/* Parsing */

void AsDijkstra::newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route)
{
  XBT_DEBUG("Load Route from \"%d\" to \"%d\"", src_id, dst_id);
  xbt_node_t src = NULL;
  xbt_node_t dst = NULL;

  graph_node_map_element_t src_elm = (graph_node_map_element_t)
          xbt_dict_get_or_null_ext(p_graphNodeMap,
              (char *) (&src_id),
              sizeof(int));
  graph_node_map_element_t dst_elm = (graph_node_map_element_t)
          xbt_dict_get_or_null_ext(p_graphNodeMap,
              (char *) (&dst_id),
              sizeof(int));


  if (src_elm)
    src = src_elm->node;

  if (dst_elm)
    dst = dst_elm->node;

  /* add nodes if they don't exist in the graph */
  if (src_id == dst_id && src == NULL && dst == NULL) {
    src = this->routeGraphNewNode(src_id, -1);
    dst = src;
  } else {
    if (src == NULL) {
      src = this->routeGraphNewNode(src_id, -1);
    }
    if (dst == NULL) {
      dst = this->routeGraphNewNode(dst_id, -1);
    }
  }

  /* add link as edge to graph */
  xbt_graph_new_edge(p_routeGraph, src, dst, e_route);
}

void AsDijkstra::addLoopback() {
  xbt_dynar_t nodes = xbt_graph_get_nodes(p_routeGraph);

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
      sg_platf_route_cbarg_t e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
      e_route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
      xbt_dynar_push(e_route->link_list, &routing_platf->p_loopback);
      xbt_graph_new_edge(p_routeGraph, node, node, e_route);
    }
  }
}

xbt_dynar_t AsDijkstra::getOnelinkRoutes()
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(OnelinkPtr), xbt_free);
  sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t,1);
  route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t),NULL);

  int src,dst;
  RoutingEdgePtr src_elm, dst_elm;
  size_t table_size = xbt_dynar_length(p_indexNetworkElm);
  for(src=0; src < table_size; src++) {
    for(dst=0; dst< table_size; dst++) {
      xbt_dynar_reset(route->link_list);
      src_elm = xbt_dynar_get_as(p_indexNetworkElm, src, RoutingEdgePtr);
      dst_elm = xbt_dynar_get_as(p_indexNetworkElm, dst, RoutingEdgePtr);
      this->getRouteAndLatency(src_elm, dst_elm,route, NULL);

      if (xbt_dynar_length(route->link_list) == 1) {
        void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
        OnelinkPtr onelink = new Onelink();
        onelink->p_linkPtr = link;
        if (p_hierarchy == SURF_ROUTING_BASE) {
          onelink->p_src = src_elm;
          onelink->p_dst = dst_elm;
        } else if (p_hierarchy == SURF_ROUTING_RECURSIVE) {
          onelink->p_src = route->gw_src;
          onelink->p_dst = route->gw_dst;
        }
        xbt_dynar_push(ret, &onelink);
      }
    }
  }
  return ret;
}

void AsDijkstra::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t route, double *lat)
{

  /* set utils vars */

  srcDstCheck(src, dst);
  int *src_id = &(src->m_id);
  int *dst_id = &(dst->m_id);

  if (!src_id || !dst_id)
    THROWF(arg_error,0,"No route from '%s' to '%s'",src->p_name,dst->p_name);

  int *pred_arr = NULL;
  int src_node_id = 0;
  int dst_node_id = 0;
  int *nodeid = NULL;
  int v;
  sg_platf_route_cbarg_t e_route;
  int size = 0;
  unsigned int cpt;
  void *link;
  xbt_dynar_t links = NULL;
  route_cache_element_t elm = NULL;
  xbt_dynar_t nodes = xbt_graph_get_nodes(p_routeGraph);

  /* Use the graph_node id mapping set to quickly find the nodes */
  graph_node_map_element_t src_elm = nodeMapSearch(*src_id);
  graph_node_map_element_t dst_elm = nodeMapSearch(*dst_id);

  src_node_id = ((graph_node_data_t)
      xbt_graph_node_get_data(src_elm->node))->graph_id;
  dst_node_id = ((graph_node_data_t)
      xbt_graph_node_get_data(dst_elm->node))->graph_id;

  /* if the src and dst are the same */
  if (src_node_id == dst_node_id) {

    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_node_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_node_id, xbt_node_t);
    xbt_edge_t edge = xbt_graph_get_edge(p_routeGraph, node_s_v, node_e_v);

    if (edge == NULL)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->p_name, dst->p_name);

    e_route = (sg_platf_route_cbarg_t) xbt_graph_edge_get_data(edge);

    links = e_route->link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(route->link_list, &link);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(link))->getLatency();
    }

  }

  if (m_cached) {
    /*check if there is a cached predecessor list avail */
    elm = (route_cache_element_t)
            xbt_dict_get_or_null_ext(p_routeCache, (char *) (&src_id),
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
      int *v_id = (int *) xbt_heap_pop(pqueue);
      xbt_node_t v_node = xbt_dynar_get_as(nodes, *v_id, xbt_node_t);
      xbt_dynar_t out_edges = xbt_graph_node_get_outedges(v_node);
      xbt_edge_t edge = NULL;
      unsigned int cursor;

      xbt_dynar_foreach(out_edges, cursor, edge) {
        xbt_node_t u_node = xbt_graph_edge_get_target(edge);
        graph_node_data_t data = (graph_node_data_t) xbt_graph_node_get_data(u_node);
        int u_id = data->graph_id;
        sg_platf_route_cbarg_t tmp_e_route = (sg_platf_route_cbarg_t) xbt_graph_edge_get_data(edge);
        int cost_v_u = (tmp_e_route->link_list)->used;    /* count of links, old model assume 1 */

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
  RoutingEdgePtr gw_src = NULL, gw_dst, prev_gw_src, first_gw = NULL;
  RoutingEdgePtr gw_dst_net_elm = NULL, prev_gw_src_net_elm = NULL;

  for (v = dst_node_id; v != src_node_id; v = pred_arr[v]) {
    xbt_node_t node_pred_v =
        xbt_dynar_get_as(nodes, pred_arr[v], xbt_node_t);
    xbt_node_t node_v = xbt_dynar_get_as(nodes, v, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(p_routeGraph, node_pred_v, node_v);

    if (edge == NULL)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->p_name, dst->p_name);

    prev_gw_src = gw_src;

    e_route = (sg_platf_route_cbarg_t) xbt_graph_edge_get_data(edge);
    gw_src = e_route->gw_src;
    gw_dst = e_route->gw_dst;

    if (v == dst_node_id)
      first_gw = gw_dst;

    if (p_hierarchy == SURF_ROUTING_RECURSIVE && v != dst_node_id
        && strcmp(gw_dst->p_name, prev_gw_src->p_name)) {
      xbt_dynar_t e_route_as_to_as=NULL;

      routing_platf->getRouteAndLatency(gw_dst_net_elm, prev_gw_src_net_elm, &e_route_as_to_as, NULL);
      if (edge == NULL)
        THROWF(arg_error,0,"No route from '%s' to '%s'", src->p_name, dst->p_name);
      links = e_route_as_to_as;
      int pos = 0;
      xbt_dynar_foreach(links, cpt, link) {
        xbt_dynar_insert_at(route->link_list, pos, &link);
        if (lat)
          *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(link))->getLatency();
        pos++;
      }
    }

    links = e_route->link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_unshift(route->link_list, &link);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(link))->getLatency();
    }
    size++;
  }

  if (p_hierarchy == SURF_ROUTING_RECURSIVE) {
    route->gw_src = gw_src;
    route->gw_dst = first_gw;
  }

  if (m_cached && elm == NULL) {
    /* add to predecessor list of the current src-host to cache */
    elm = xbt_new0(struct route_cache_element, 1);
    elm->pred_arr = pred_arr;
    elm->size = size;
    xbt_dict_set_ext(p_routeCache, (char *) (&src_id), sizeof(int),
        (xbt_set_elm_t) elm, NULL);
  }

  if (!m_cached)
    xbt_free(pred_arr);
}

AsDijkstra::~AsDijkstra()
{
  xbt_graph_free_graph(p_routeGraph, &xbt_free,
      &graph_edge_data_free, &xbt_free);
  xbt_dict_free(&p_graphNodeMap);
  if (m_cached)
    xbt_dict_free(&p_routeCache);
}

/* Creation routing model functions */

AsDijkstra::AsDijkstra() : AsGeneric(), m_cached(0) {}

AsDijkstra::AsDijkstra(int cached) : AsGeneric(), m_cached(cached)
{
  /*new_component->generic_routing.parse_route = model_dijkstra_both_parse_route;
  new_component->generic_routing.parse_ASroute = model_dijkstra_both_parse_route;
  new_component->generic_routing.get_route_and_latency = dijkstra_get_route_and_latency;
  new_component->generic_routing.get_onelink_routes =
      dijkstra_get_onelink_routes;
  new_component->generic_routing.get_graph = generic_get_graph;
  new_component->generic_routing.finalize = dijkstra_finalize;
  new_component->cached = cached;*/
}

void AsDijkstra::end()
{
  xbt_node_t node = NULL;
  unsigned int cursor2;
  xbt_dynar_t nodes = NULL;

  /* Create the topology graph */
  if(!p_routeGraph)
	p_routeGraph = xbt_graph_new_graph(1, NULL);
  if(!p_graphNodeMap)
    p_graphNodeMap = xbt_dict_new_homogeneous(&graph_node_map_elem_free);

  if (m_cached && p_routeCache)
    p_routeCache = xbt_dict_new_homogeneous(&route_cache_elem_free);

  /* Add the loopback if needed */
  if (routing_platf->p_loopback && p_hierarchy == SURF_ROUTING_BASE)
    addLoopback();

  /* initialize graph indexes in nodes after graph has been built */
  nodes = xbt_graph_get_nodes(p_routeGraph);

  xbt_dynar_foreach(nodes, cursor2, node) {
    graph_node_data_t data = (graph_node_data_t) xbt_graph_node_get_data(node);
    data->graph_id = cursor2;
  }

}
void AsDijkstra::parseRoute(sg_platf_route_cbarg_t route)
{
  char *src = (char*)(route->src);
  char *dst = (char*)(route->dst);

  int as_route = 0;
  if(!route->gw_dst && !route->gw_src)
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
  else{
    XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
        route->gw_src->p_name, dst, route->gw_dst->p_name);
    as_route = 1;
    if(route->gw_dst->p_rcType == SURF_NETWORK_ELEMENT_NULL)
      xbt_die("The gw_dst '%s' does not exist!",route->gw_dst->p_name);
    if(route->gw_src->p_rcType == SURF_NETWORK_ELEMENT_NULL)
      xbt_die("The gw_src '%s' does not exist!",route->gw_src->p_name);
  }

  RoutingEdgePtr src_net_elm, dst_net_elm;

  src_net_elm = sg_routing_edge_by_name_or_null(src);
  dst_net_elm = sg_routing_edge_by_name_or_null(dst);

  xbt_assert(src_net_elm, "Network elements %s not found", src);
  xbt_assert(dst_net_elm, "Network elements %s not found", dst);

  /* Create the topology graph */
  if(!p_routeGraph)
    p_routeGraph = xbt_graph_new_graph(1, NULL);
  if(!p_graphNodeMap)
    p_graphNodeMap = xbt_dict_new_homogeneous(&graph_node_map_elem_free);

  if (m_cached && !p_routeCache)
    p_routeCache = xbt_dict_new_homogeneous(&route_cache_elem_free);

  sg_platf_route_cbarg_t e_route = newExtendedRoute(p_hierarchy, route, 1);
  newRoute(src_net_elm->m_id, dst_net_elm->m_id, e_route);

  // Symmetrical YES
  if ( (route->symmetrical == TRUE && as_route == 0)
      || (route->symmetrical == TRUE && as_route == 1)
  )
  {
    if(!route->gw_dst && !route->gw_src)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
    else
      XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
          route->gw_dst->p_name, src, route->gw_src->p_name);

    xbt_dynar_t nodes = xbt_graph_get_nodes(p_routeGraph);
    xbt_node_t node_s_v = xbt_dynar_get_as(nodes, src_net_elm->m_id, xbt_node_t);
    xbt_node_t node_e_v = xbt_dynar_get_as(nodes, dst_net_elm->m_id, xbt_node_t);
    xbt_edge_t edge =
        xbt_graph_get_edge(p_routeGraph, node_e_v, node_s_v);

    if (edge)
      THROWF(arg_error,0,"(AS)Route from '%s' to '%s' already exists",src,dst);

    if (route->gw_dst && route->gw_src) {
      RoutingEdgePtr gw_tmp;
      gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }
    sg_platf_route_cbarg_t link_route_back = newExtendedRoute(p_hierarchy, route, 0);
    newRoute(dst_net_elm->m_id, src_net_elm->m_id, link_route_back);
  }
  xbt_dynar_free(&route->link_list);
}
