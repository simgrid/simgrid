/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "surf_routing.hpp"

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsNone : public As {
public:
  AsNone() {}
  void Seal() override {}; // nothing to do
  ~AsNone() {}

  void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
  xbt_dynar_t getOneLinkRoutes() override;
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
  sg_platf_route_cbarg_t getBypassRoute(NetCard *src, NetCard *dst, double *lat) override;

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int parsePU(NetCard *elm) override; /* A host or a router, whatever */
  int parseAS( NetCard *elm) override;
  void parseRoute(sg_platf_route_cbarg_t route) override;
  void parseASroute(sg_platf_route_cbarg_t route) override;
  void parseBypassroute(sg_platf_route_cbarg_t e_route) override;
};

}
}

#endif /* SURF_ROUTING_NONE_HPP_ */
