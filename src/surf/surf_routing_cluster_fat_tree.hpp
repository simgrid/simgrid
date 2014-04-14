/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_cluster.hpp"

#ifndef SURF_ROUTING_CLUSTER_FAT_TREE_HPP_
#define SURF_ROUTING_CLUSTER_FAT_TREE_HPP_



class FatTreeLink;
class FatTreeNode;

class AsClusterFatTree : public AsCluster {
public:
  AsClusterFatTree();
  virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  virtual void create_links();
  void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster);

protected:
  //description of a PGFT (TODO : better doc)
  unsigned int levels;
  std::vector<int> lowerLevelNodesNumber;
  std::vector<int> upperLevelNodesNumber;
  std::vector<int> lowerLevelPortsNumber;
  
  std::vector<FatTreeNode> nodes;
};

class FatTreeLink {
public:
};
class FatTreeNode {
  int id;
  std::string name;
};
  
#endif
