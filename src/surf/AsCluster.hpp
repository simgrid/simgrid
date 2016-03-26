/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_HPP_
#define SURF_ROUTING_CLUSTER_HPP_

#include "src/surf/AsImpl.hpp"

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsCluster: public AsImpl {
public:
  AsCluster(const char*name);
  ~AsCluster();

  virtual void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;

  virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
  virtual void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) {}

  xbt_dynar_t privateLinks_ = xbt_dynar_new(sizeof(s_surf_parsing_link_up_down_t),NULL);

  Link* backbone_ = nullptr;
  void *loopback_ = nullptr;
  NetCard *router_ = nullptr;
  int has_limiter_  = 0; /* O or 1. must be an int since it's used to shift the considered index */
  int has_loopback_ = 0; /* O or 1. must be an int since it's used to shift the considered index */
  int nb_links_per_node_ = 1; /* may be 1 (if only a private link), 2 or 3 (if limiter and loopback) */

};

}
}

#endif /* SURF_ROUTING_CLUSTER_HPP_ */
