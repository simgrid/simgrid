/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

#include "src/surf/AsImpl.hpp"

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsRoutedGraph : public AsImpl {
public:
  AsRoutedGraph(const char*name);
  ~AsRoutedGraph();

  xbt_dynar_t getOneLinkRoutes() override;

  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
  virtual sg_platf_route_cbarg_t newExtendedRoute(RoutingMode hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
protected:
  void getRouteCheckParams(NetCard *src, NetCard *dst);
  void addRouteCheckParams(sg_platf_route_cbarg_t route);
};

}
}

#endif /* SURF_ROUTING_GENERIC_HPP_ */
