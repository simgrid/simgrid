/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_FULL_HPP_
#define SIMGRID_ROUTING_FULL_HPP_

#include "src/kernel/routing/RoutedZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing provided by the user
 *
 *  The full communication matrix is provided at creation, so this model has the highest expressive power and the lowest
 *  computational requirements, but also the highest memory requirements (both in platform file and in memory).
 */
class XBT_PRIVATE FullZone : public RoutedZone {
public:
  explicit FullZone(NetZone* father, std::string name);
  void seal() override;
  ~FullZone() override;

  void getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void addRoute(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
                kernel::routing::NetPoint* gw_dst, std::vector<simgrid::surf::LinkImpl*>& link_list,
                bool symmetrical) override;

  sg_platf_route_cbarg_t* routingTable_ = nullptr;
};
}
}
} // namespaces

#endif /* SIMGRID_ROUTING_FULL_HPP_ */
