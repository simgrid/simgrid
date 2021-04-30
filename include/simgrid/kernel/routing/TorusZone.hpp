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
  s4u::Link::SharingPolicy link_sharing_policy_; //!< torus links: sharing policy
  double link_bw_;                               //!< torus links: bandwidth
  double link_lat_;                              //!< torus links: latency

public:
  using ClusterZone::ClusterZone;
  void create_links(int id, int rank, unsigned int position);
  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency) override;
  void set_topology(const std::vector<unsigned int>& dimensions);

  /** @brief Convert topology parameters from string to vector of uint */
  static std::vector<unsigned int> parse_topo_parameters(const std::string& topo_parameters);
  /** @brief Set the characteristics of links inside the Torus zone */
  void set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy);
};

} // namespace routing
} // namespace kernel
} // namespace simgrid
#endif
