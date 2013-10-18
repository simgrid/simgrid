/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_none.hpp"
#include "surf_routing_private.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_none, surf, "Routing part of surf");
}

AS_t model_none_create(void)
{
  return new AsNone();
}

xbt_dynar_t AsNone::getOneLinkRoutes() {
  return NULL;
}

void AsNone::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst,
    sg_platf_route_cbarg_t res, double *lat)
{
  *lat = 0.0;
}

void AsNone::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
	XBT_INFO("No routing no graph");
}

sg_platf_route_cbarg_t AsNone::getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat) {
  return NULL;
}

int AsNone::parsePU(RoutingEdgePtr elm) {
  XBT_DEBUG("Load process unit \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  /* don't care about PUs */
  return -1;
}

int AsNone::parseAS(RoutingEdgePtr elm) {
  XBT_DEBUG("Load Autonomous system \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  /* even don't care about sub-ASes -- I'm as nihilist as an old punk*/
  return -1;
}

void AsNone::parseRoute(sg_platf_route_cbarg_t route){
  THROW_IMPOSSIBLE;
}

void AsNone::parseASroute(sg_platf_route_cbarg_t route){
  THROW_IMPOSSIBLE;
}
void AsNone::parseBypassroute(sg_platf_route_cbarg_t e_route){
  THROW_IMPOSSIBLE;
}

/* Creation routing model functions */
AsNone::AsNone() {
  p_routingSons = xbt_dict_new_homogeneous(NULL);
  p_indexNetworkElm = xbt_dynar_new(sizeof(char*),NULL);
}

AsNone::~AsNone() {
  xbt_dict_free(&p_routingSons);
  xbt_dynar_free(&p_indexNetworkElm);
  xbt_dynar_free(&p_linkUpDownList);
}

