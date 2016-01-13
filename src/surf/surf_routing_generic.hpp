/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "surf_routing_none.hpp"

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsGeneric;

class XBT_PRIVATE AsGeneric : public AsNone {
public:
  AsGeneric();
  ~AsGeneric();

  virtual void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
  virtual sg_platf_route_cbarg_t getBypassRoute(NetCard *src, NetCard *dst, double *lat) override;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(NetCard *elm) override; /* A host or a router, whatever */
  virtual int parseAS(NetCard *elm) override;
  virtual void parseRoute(sg_platf_route_cbarg_t route) override;
  virtual void parseASroute(sg_platf_route_cbarg_t route) override;
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route) override;

  virtual sg_platf_route_cbarg_t newExtendedRoute(e_surf_routing_hierarchy_t hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
  virtual As *asExist(As *to_find);
  virtual As *autonomousSystemExist(char *element);
  virtual As *processingUnitsExist(char *element);
  virtual void srcDstCheck(NetCard *src, NetCard *dst);
};

}
}

#endif /* SURF_ROUTING_GENERIC_HPP_ */
