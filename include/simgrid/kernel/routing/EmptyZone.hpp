/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <xbt/asserts.h>

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 *  @brief NetZone with no routing, useful with the constant network model
 *
 *  Such netzones never contain any link, and the latency is always left unchanged:
 *  the constant time network model computes this latency externally.
 */

class XBT_PRIVATE EmptyZone : public NetZoneImpl {
public:
  explicit EmptyZone(const std::string& name) : NetZoneImpl(name) {}

  void get_local_route(const NetPoint* src, const NetPoint* dst, Route* into, double* latency) override
  {
    xbt_die("There can't be route in an Empty zone");
  }

  void get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* /*nodes*/,
                 std::map<std::string, xbt_edge_t, std::less<>>* /*edges*/) override;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid

#endif /* SURF_ROUTING_NONE_HPP_ */
