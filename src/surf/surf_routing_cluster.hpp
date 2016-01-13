/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_HPP_
#define SURF_ROUTING_CLUSTER_HPP_

#include <xbt/base.h>

#include "surf_routing_none.hpp"
#include "network_interface.hpp"

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class XBT_PRIVATE AsCluster;

/* ************************************************** */
/* **************  Cluster ROUTING   **************** */

class AsCluster: public AsNone {
public:
  AsCluster() {}

  virtual void getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t into, double *latency) override;
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) override;

  int parsePU(NetCard *elm) override; /* A host or a router, whatever */
  int parseAS(NetCard *elm) override;
  virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);

  Link* p_backbone = nullptr;
  void *p_loopback = nullptr;
  NetCard *p_router = nullptr;
  int p_has_limiter = 0;
  int p_has_loopback = 0;
  int p_nb_links_per_node = 1;

};

}
}

#endif /* SURF_ROUTING_CLUSTER_HPP_ */
