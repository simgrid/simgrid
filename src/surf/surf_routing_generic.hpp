/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "surf_routing.hpp"

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsGeneric;

class XBT_PRIVATE AsGeneric : public As {
public:
  AsGeneric(const char*name);
  ~AsGeneric();

  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
  virtual sg_platf_route_cbarg_t getBypassRoute(NetCard *src, NetCard *dst, double *lat) override;

  /* Add content to the AS, at parsing time */
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route) override;

  virtual sg_platf_route_cbarg_t newExtendedRoute(e_surf_routing_hierarchy_t hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
protected:
  void getRouteCheckParams(NetCard *src, NetCard *dst);
  void addRouteCheckParams(sg_platf_route_cbarg_t route);
private:
  xbt_dict_t bypassRoutes_ = xbt_dict_new_homogeneous((void (*)(void *)) routing_route_free);
};

}
}

#endif /* SURF_ROUTING_GENERIC_HPP_ */
