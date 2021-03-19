/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_TORUS_HPP_
#define SURF_ROUTING_CLUSTER_TORUS_HPP_

#include <simgrid/kernel/routing/ClusterZone.hpp>

#include <vector>

namespace simgrid {
namespace kernel {
namespace routing {

/** @ingroup ROUTING_API
 * @brief NetZone using a Torus topology
 *
 */

class XBT_PRIVATE TorusZone : public ClusterZone {
  std::vector<unsigned int> dimensions_;

public:
  explicit TorusZone(const std::string& name) : ClusterZone(name){};
  void create_links_for_node(ClusterCreationArgs* cluster, int id, int rank, unsigned int position) override;
  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency) override;
  void parse_specific_arguments(ClusterCreationArgs* cluster) override;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid
#endif
