/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_FULL_HPP_
#define SIMGRID_ROUTING_FULL_HPP_

#include "src/kernel/routing/AsRoutedGraph.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** Full routing: fast, large memory requirements, fully expressive */
class XBT_PRIVATE AsFull: public AsRoutedGraph {
public:
  explicit AsFull(As* father, const char* name);
  void seal() override;
  ~AsFull() override;

  void getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void addRoute(sg_platf_route_cbarg_t route) override;

  sg_platf_route_cbarg_t *routingTable_ = nullptr;
};

}}} // namespaces

#endif /* SIMGRID_ROUTING_FULL_HPP_ */
