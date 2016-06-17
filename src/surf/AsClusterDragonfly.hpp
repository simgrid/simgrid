/* Copyright (c) 2014-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_
#define SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_

#include "src/surf/AsCluster.hpp"

namespace simgrid {
  namespace surf {


class XBT_PRIVATE DragonflyRouter {
    public:
      int group_;
      int chassis_;
      int blade_;
      Link** blueLinks_=NULL;
      Link** blackLinks_=NULL;
      Link** greenLinks_=NULL;
      Link** myNodes_=NULL;
      DragonflyRouter(int i, int j, int k);
      ~DragonflyRouter();
};


class XBT_PRIVATE AsClusterDragonfly:public simgrid::surf::AsCluster {
    public:
      explicit AsClusterDragonfly(const char*name);
      ~AsClusterDragonfly() override;
//      void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position) override;
      void getRouteAndLatency(NetCard * src, NetCard * dst, sg_platf_route_cbarg_t into, double *latency) override;
      void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) override;
      void seal() override;
      void generateRouters();
      void generateLinks();
      void createLink(char* id, Link** linkup, Link** linkdown);
      unsigned int * rankId_to_coords(int rankId);
    private:
      sg_platf_cluster_cbarg_t cluster_;
      unsigned int numNodesPerBlade_ = 0;
      unsigned int numBladesPerChassis_ = 0;
      unsigned int numChassisPerGroup_ = 0;
      unsigned int numGroups_ = 0;
      unsigned int numLinksGreen_ = 0;
      unsigned int numLinksBlack_ = 0;
      unsigned int numLinksBlue_ = 0;
      DragonflyRouter** routers_=NULL;
    };

  }}
#endif
