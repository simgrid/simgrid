/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "surf_routing_none.hpp"

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

class XBT_PRIVATE AsGeneric;

XBT_PRIVATE void generic_free_route(sg_platf_route_cbarg_t route);

class XBT_PRIVATE AsGeneric : public AsNone {
public:
  AsGeneric();
  ~AsGeneric();

  virtual void getRouteAndLatency(RoutingEdge *src, RoutingEdge *dst, sg_platf_route_cbarg_t into, double *latency);
  virtual xbt_dynar_t getOneLinkRoutes();
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  virtual sg_platf_route_cbarg_t getBypassRoute(RoutingEdge *src, RoutingEdge *dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(RoutingEdge *elm); /* A host or a router, whatever */
  virtual int parseAS(RoutingEdge *elm);
  virtual void parseRoute(sg_platf_route_cbarg_t route);
  virtual void parseASroute(sg_platf_route_cbarg_t route);
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route);

  virtual sg_platf_route_cbarg_t newExtendedRoute(e_surf_routing_hierarchy_t hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
  virtual As *asExist(As *to_find);
  virtual As *autonomousSystemExist(char *element);
  virtual As *processingUnitsExist(char *element);
  virtual void srcDstCheck(RoutingEdge *src, RoutingEdge *dst);
};

#endif /* SURF_ROUTING_GENERIC_HPP_ */
