/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_
#define SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_

#include "src/kernel/routing/ClusterZone.hpp"

namespace simgrid {
namespace kernel {
namespace routing {

class XBT_PRIVATE DragonflyRouter {
public:
  unsigned int group_;
  unsigned int chassis_;
  unsigned int blade_;
  surf::LinkImpl** blueLinks_  = nullptr;
  surf::LinkImpl** blackLinks_ = nullptr;
  surf::LinkImpl** greenLinks_ = nullptr;
  surf::LinkImpl** myNodes_    = nullptr;
  DragonflyRouter(int i, int j, int k);
  ~DragonflyRouter();
};

/** @ingroup ROUTING_API
 * @brief NetZone using a Dragonfly topology
 *
 * Generate dragonfly according to the topology asked for, according to:
 * Cray Cascade: a Scalable HPC System based on a Dragonfly Network
 * Greg Faanes, Abdulla Bataineh, Duncan Roweth, Tom Court, Edwin Froese,
 * Bob Alverson, Tim Johnson, Joe Kopnick, Mike Higgins and James Reinhard
 * Cray Inc, Chippewa Falls, Wisconsin, USA
 * or http://www.cray.com/sites/default/files/resources/CrayXCNetwork.pdf
 *
 * We use the same denomination for the different levels, with a Green,
 * Black and Blue color scheme for the three different levels.
 *
 * Description of the topology has to be given with a string of type :
 * "3,4;4,3;5,1;2"
 *
 * Last part  : "2"   : 2 nodes per blade
 * Third part : "5,1" : five blades/routers per chassis, with one link between each (green network)
 * Second part : "4,3" = four chassis per group, with three links between each nth router of each chassis (black
 * network)
 * First part : "3,4" = three electrical groups, linked in an alltoall
 * pattern by 4 links each (blue network)
 *
 * LIMITATIONS (for now):
 *  - Routing is only static and uses minimal routes.
 *  - When n links are used between two routers/groups, we consider only one link with n times the bandwidth (needs to
 *    be validated on a real system)
 *  - All links have the same characteristics for now
 *  - Blue links are all attached to routers in the chassis n°0. This limits
 *    the number of groups possible to the number of blades in a chassis. This
 *    is also not realistic, as blue level can use more links than a single
 *    Aries can handle, thus it should use several routers.
 */
class XBT_PRIVATE DragonflyZone : public ClusterZone {
public:
  explicit DragonflyZone(NetZone* father, std::string name);
  ~DragonflyZone() override;
  //      void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position) override;
  void getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t into, double* latency) override;
  void parse_specific_arguments(ClusterCreationArgs* cluster) override;
  void seal() override;
  void generateRouters();
  void generateLinks();
  void createLink(const std::string& id, int numlinks, surf::LinkImpl** linkup, surf::LinkImpl** linkdown);

private:
  void rankId_to_coords(int rankId, unsigned int (*coords)[4]);
  ClusterCreationArgs* cluster_     = nullptr;
  unsigned int numNodesPerBlade_    = 0;
  unsigned int numBladesPerChassis_ = 0;
  unsigned int numChassisPerGroup_  = 0;
  unsigned int numGroups_           = 0;
  unsigned int numLinksGreen_       = 0;
  unsigned int numLinksBlack_       = 0;
  unsigned int numLinksBlue_        = 0;
  unsigned int numLinksperLink_     = 1; // fullduplex -> 2, only for local link
  DragonflyRouter** routers_        = nullptr;
};
}
}
}
#endif
