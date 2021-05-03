/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_FULL_HPP_
#define SIMGRID_ROUTING_FULL_HPP_

#include <simgrid/kernel/routing/RoutedZone.hpp>

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
  std::vector<Route*> routing_table_;
  void do_seal() override;

public:
  using RoutedZone::RoutedZone;
  FullZone(const FullZone&) = delete;
  FullZone& operator=(const FullZone) = delete;
  ~FullZone() override;

  void get_local_route(NetPoint* src, NetPoint* dst, Route* into, double* latency) override;
  void add_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                 const std::vector<resource::LinkImpl*>& link_list, bool symmetrical) override;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_ROUTING_FULL_HPP_ */
