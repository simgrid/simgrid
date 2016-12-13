/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

#include "src/kernel/routing/NetZoneImpl.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** No specific routing. Mainly useful with the constant network model */
class XBT_PRIVATE EmptyZone : public NetZoneImpl {
public:
  explicit EmptyZone(NetZone* father, const char* name);
  ~EmptyZone() override;

  void getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;
};
}
}
} // namespace

#endif /* SURF_ROUTING_NONE_HPP_ */
