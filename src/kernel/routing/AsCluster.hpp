/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_ROUTING_CLUSTER_HPP_
#define SIMGRID_ROUTING_CLUSTER_HPP_

#include "src/kernel/routing/AsImpl.hpp"

namespace simgrid {
namespace routing {

class XBT_PRIVATE AsCluster: public AsImpl {
public:
  explicit AsCluster(const char*name);
  ~AsCluster() override;

  void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;

  virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
  virtual void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) {}

  xbt_dynar_t privateLinks_ = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),nullptr);

  Link* backbone_ = nullptr;
  void *loopback_ = nullptr;
  NetCard *router_ = nullptr;
  bool hasLimiter_  = false;
  bool hasLoopback_ = false;
  int linkCountPerNode_ = 1; /* may be 1 (if only a private link), 2 or 3 (if limiter and loopback) */

};

}
}

#endif /* SIMGRID_ROUTING_CLUSTER_HPP_ */
