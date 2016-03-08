/* Copyright (c) 2014-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_TORUS_HPP_
#define SURF_ROUTING_CLUSTER_TORUS_HPP_

#include "src/surf/AsCluster.hpp"

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE AsClusterTorus:public simgrid::surf::AsCluster {
    public:
      AsClusterTorus(const char*name);
      virtual ~AsClusterTorus();
      void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position) override;
      void getRouteAndLatency(NetCard * src, NetCard * dst, sg_platf_route_cbarg_t into, double *latency) override;
      void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) override;
    private:
      xbt_dynar_t dimensions_ = nullptr;
    };

  }}
#endif
