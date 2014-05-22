/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_cluster.hpp"

#ifndef SURF_ROUTING_CLUSTER_FAT_TREE_HPP_
#define SURF_ROUTING_CLUSTER_FAT_TREE_HPP_


/* The class AsClusterFatTree describes PGFT, as introduced by Eitan Zahavi
 * in "D-Mod-K Routing Providing Non-Blocking Traffic for Shift Permutations
 * on Real Life Fat Trees" (2010). RLFT are PGFT with some restrictions to 
 * address real world constraints, which are not currently enforced (but it 
 * should certainly be checked for)
 */

/* TODO : limiter link ? Loopback?
 *
 */
class FatTreeNode;
class FatTreeLink;


class FatTreeNode {
public:
  int id;
  unsigned int level; // The 0th level represents the leafs of the PGFT
  unsigned int position; // Position in the level
  std::vector<unsigned int> label;
  /* We can see the sizes sum of the two following vectors as the device 
   * ports number. If we use the notations used in Zahavi's paper, 
   * children.size() = m_level and parents.size() = w_(level+1)
   * 
   */
  std::vector<FatTreeLink*> children;  // m, apply from lvl 0 to levels - 1 
  std::vector<FatTreeLink*> parents; // w, apply from lvl 1 to levels
  FatTreeNode(int id, int level=-1, int position=-1);
};

class FatTreeLink {
public:
  FatTreeLink(sg_platf_cluster_cbarg_t cluster, FatTreeNode *source,
              FatTreeNode *destination);
  //  unsigned int ports;
  /* Links are dependant of the chosen network model, but must implement 
   * NetworkLink
   */
  NetworkLink *upLink; 
  NetworkLink *downLink;
  FatTreeNode *upNode;
  FatTreeNode *downNode;
  
};

class AsClusterFatTree : public AsCluster {
public:
  AsClusterFatTree();
  ~AsClusterFatTree();
  virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst,
                                  sg_platf_route_cbarg_t into,
                                  double *latency);
  // virtual void getRouteAndLatency(const int src, const int dst,
  //                                 std::vector<NetworkLink> *route,
  //                                 double *latency) const;
  virtual void create_links(sg_platf_cluster_cbarg_t cluster);
  void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster);
  void addProcessingNode(int id);
  void generateDotFile(const string& filename = "fatTree.dot") const;

protected:
  //description of a PGFT (TODO : better doc)
  unsigned int levels;
  std::vector<unsigned int> lowerLevelNodesNumber; // number of children by node
  std::vector<unsigned int> upperLevelNodesNumber; // number of parents by node
  std::vector<unsigned int> lowerLevelPortsNumber; // ports between each level l and l-1
  
  std::map<int, FatTreeNode*> computeNodes;
  std::vector<FatTreeNode*> nodes;
  std::vector<FatTreeLink*> links;
  std::vector<unsigned int> nodesByLevel;

  void addLink(sg_platf_cluster_cbarg_t cluster, 
               FatTreeNode *parent, unsigned int parentPort,
               FatTreeNode *child, unsigned int childPort);
  int getLevelPosition(const unsigned int level);
  void generateLabels();
  void generateSwitches();
  int connectNodeToParents(sg_platf_cluster_cbarg_t cluster, FatTreeNode *node);
  bool areRelated(FatTreeNode *parent, FatTreeNode *child);
  bool isInSubTree(FatTreeNode *root, FatTreeNode *node);
};
#endif
