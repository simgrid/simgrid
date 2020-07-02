/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_
#define SURF_ROUTING_CLUSTER_DRAGONFLY_HPP_

#include <simgrid/kernel/routing/ClusterZone.hpp>
#include <simgrid/s4u/Link.hpp>

namespace simgrid {
namespace kernel {
namespace routing {

class DragonflyRouter {
public:
  unsigned int group_;
  unsigned int chassis_;
  unsigned int blade_;
  resource::LinkImpl* blue_link_ = nullptr;
  std::vector<resource::LinkImpl*> black_links_;
  std::vector<resource::LinkImpl*> green_links_;
  std::vector<resource::LinkImpl*> my_nodes_;
  DragonflyRouter(unsigned group, unsigned chassis, unsigned blade) : group_(group), chassis_(chassis), blade_(blade) {}
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
class XBT_PUBLIC DragonflyZone : public ClusterZone {
public:
  explicit DragonflyZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel);
  void get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency) override;
  void parse_specific_arguments(ClusterCreationArgs* cluster) override;
  void seal() override;

  void rankId_to_coords(int rank_id, unsigned int coords[4]) const;

private:
  void generate_routers();
  void generate_links();
  void create_link(const std::string& id, int numlinks, resource::LinkImpl** linkup,
                   resource::LinkImpl** linkdown) const;

  simgrid::s4u::Link::SharingPolicy sharing_policy_;
  double bw_  = 0;
  double lat_ = 0;

  unsigned int num_nodes_per_blade_    = 0;
  unsigned int num_blades_per_chassis_ = 0;
  unsigned int num_chassis_per_group_  = 0;
  unsigned int num_groups_             = 0;
  unsigned int num_links_green_        = 0;
  unsigned int num_links_black_        = 0;
  unsigned int num_links_blue_         = 0;
  unsigned int num_links_per_link_     = 1; // splitduplex -> 2, only for local link
  std::vector<DragonflyRouter> routers_;
};
} // namespace routing
} // namespace kernel
} // namespace simgrid
#endif
