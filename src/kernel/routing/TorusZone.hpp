/* Copyright (c) 2014-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_TORUS_HPP_
#define SURF_ROUTING_CLUSTER_TORUS_HPP_

#include "src/kernel/routing/ClusterZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 * @brief NetZone using a Torus topology
 *
 */

class XBT_PRIVATE TorusZone : public ClusterZone {
public:
  explicit TorusZone(NetZone* father, const char* name);
  ~TorusZone() override;
  void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position) override;
  void getLocalRoute(NetCard* src, NetCard* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) override;

private:
  xbt_dynar_t dimensions_ = nullptr;
};
}
}
}
#endif
