/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_FLOYD_HPP_
#define SURF_ROUTING_FLOYD_HPP_

#include "src/kernel/routing/RoutedZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with an explicit routing computed at initialization with Floyd-Warshal
 *
 *  The path between components is computed at creation time from every one-hop links,
 *  using the Floyd-Warshal algorithm.
 *
 *  This result in rather small platform file, slow initialization time,  and intermediate memory requirements
 *  (somewhere between the one of @{DijkstraZone} and the one of @{FullZone}).
 */
class XBT_PRIVATE FloydZone : public RoutedZone {
public:
  explicit FloydZone(NetZone* father, std::string name);
  ~FloydZone() override;

  void getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void addRoute(kernel::routing::NetPoint* src, kernel::routing::NetPoint* dst, kernel::routing::NetPoint* gw_src,
                kernel::routing::NetPoint* gw_dst, std::vector<simgrid::surf::LinkImpl*>& link_list,
                bool symmetrical) override;
  void seal() override;

private:
  /* vars to compute the Floyd algorithm. */
  int* predecessorTable_;
  double* costTable_;
  sg_platf_route_cbarg_t* linkTable_;
};
}
}
} // namespaces

#endif /* SURF_ROUTING_FLOYD_HPP_ */
