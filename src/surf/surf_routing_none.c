/* Copyright (c) 2009-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");

static xbt_dynar_t none_get_onelink_routes(AS_t rc) {
  return NULL;
}

static void none_get_route_and_latency(AS_t rc, sg_routing_edge_t src, sg_routing_edge_t dst,
    sg_platf_route_cbarg_t res,double *lat)
{
  *lat = 0.0;
}

static void none_get_graph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges, AS_t rc)
{
	XBT_INFO("No routing no graph");
}

static sg_platf_route_cbarg_t none_get_bypass_route(AS_t rc,
    sg_routing_edge_t src,
    sg_routing_edge_t dst, double *lat) {
  return NULL;
}

static int none_parse_PU(AS_t rc, sg_routing_edge_t elm) {
  XBT_DEBUG("Load process unit \"%s\"", elm->name);
  xbt_dynar_push_as(rc->index_network_elm,sg_routing_edge_t,elm);
  /* don't care about PUs */
  return -1;
}

static int none_parse_AS(AS_t rc, sg_routing_edge_t elm) {
  XBT_DEBUG("Load Autonomous system \"%s\"", elm->name);
  xbt_dynar_push_as(rc->index_network_elm,sg_routing_edge_t,elm);
  /* even don't care about sub-ASes -- I'm as nihilist as an old punk*/
  return -1;
}

/* Creation routing model functions */
AS_t model_none_create() {
  return model_none_create_sized(sizeof(s_as_t));
}
AS_t model_none_create_sized(size_t childsize) {
  AS_t new_component = xbt_malloc0(childsize);
  new_component->parse_PU = none_parse_PU;
  new_component->parse_AS = none_parse_AS;
  new_component->parse_route = NULL;
  new_component->parse_ASroute = NULL;
  new_component->parse_bypassroute = NULL;
  new_component->get_route_and_latency = none_get_route_and_latency;
  new_component->get_onelink_routes = none_get_onelink_routes;
  new_component->get_bypass_route = none_get_bypass_route;
  new_component->finalize = model_none_finalize;
  new_component->get_graph = none_get_graph;
  new_component->routing_sons = xbt_dict_new_homogeneous(NULL);
  new_component->index_network_elm = xbt_dynar_new(sizeof(char*),NULL);

  return new_component;
}

void model_none_finalize(AS_t as) {
  xbt_dict_free(&as->routing_sons);
  xbt_dynar_free(&as->index_network_elm);
  xbt_dynar_free(&as->link_up_down_list);
  xbt_free(as);
}

