/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_CLUSTER_TORUS_HPP_
#define SIMGRID_ROUTING_CLUSTER_TORUS_HPP_

#include <simgrid/kernel/routing/ClusterZone.hpp>

#include <vector>

namespace simgrid::kernel::routing {

/** @ingroup ROUTING_API
 * @brief NetZone using a Torus topology
 *
 */

class XBT_PRIVATE TorusZone : public ClusterBase {
  std::vector<unsigned long> dimensions_;

public:
  explicit TorusZone(const std::string& name) : ClusterBase(name){};
  void create_torus_links(unsigned long id, int rank, unsigned long position);
  void get_local_route(const NetPoint* src, const NetPoint* dst, Route* into, double* latency) override;
  void set_topology(const std::vector<unsigned long>& dimensions);
  void do_seal() override;
};

} // namespace simgrid::kernel::routing
#endif
