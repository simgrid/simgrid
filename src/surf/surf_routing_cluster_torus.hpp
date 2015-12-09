/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef SURF_ROUTING_CLUSTER_TORUS_HPP_
#define SURF_ROUTING_CLUSTER_TORUS_HPP_

#include <xbt/base.h>

#include "surf_routing_none.hpp"
#include "network_interface.hpp"
#include "surf_routing_cluster.hpp"

namespace simgrid {
namespace surf {

class XBT_PRIVATE AsClusterTorus: public simgrid::surf::AsCluster {
public:
   AsClusterTorus();
   virtual ~AsClusterTorus();
   virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
   virtual void getRouteAndLatency(RoutingEdge *src, RoutingEdge *dst, sg_platf_route_cbarg_t into, double *latency);
   void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster);
   xbt_dynar_t p_dimensions;
};

}
}

#endif
