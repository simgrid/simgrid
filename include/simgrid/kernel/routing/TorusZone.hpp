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

class XBT_PRIVATE TorusZone : public ClusterBase {
  std::vector<unsigned long> dimensions_;

public:
  explicit TorusZone(const std::string& name) : ClusterBase(name){};
  void create_torus_links(int id, int rank, unsigned long position);
  void get_local_route(const NetPoint* src, const NetPoint* dst, Route* into, double* latency) override;
  void set_topology(const std::vector<unsigned long>& dimensions);

  /** @brief Convert topology parameters from string to vector of uint */
  static std::vector<unsigned long> parse_topo_parameters(const std::string& topo_parameters);
};

} // namespace routing
} // namespace kernel
} // namespace simgrid
#endif
