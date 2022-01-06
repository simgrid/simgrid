/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

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
  resource::StandardLinkImpl* blue_link_ = nullptr;
  resource::StandardLinkImpl* limiter_   = nullptr;
  std::vector<resource::StandardLinkImpl*> black_links_;
  std::vector<resource::StandardLinkImpl*> green_links_;
  std::vector<resource::StandardLinkImpl*> my_nodes_;
  DragonflyRouter(unsigned group, unsigned chassis, unsigned blade, resource::StandardLinkImpl* limiter)
      : group_(group), chassis_(chassis), blade_(blade), limiter_(limiter)
  {
  }
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
class XBT_PUBLIC DragonflyZone : public ClusterBase {
public:
  struct Coords {
    unsigned long group;
    unsigned long chassis;
    unsigned long blade;
    unsigned long node;
  };

  explicit DragonflyZone(const std::string& name);
  void get_local_route(const NetPoint* src, const NetPoint* dst, Route* into, double* latency) override;
  /**
   * @brief Parse topology parameters from string format
   *
   * @param topo_parameters Topology parameters, e.g. "3,4 ; 3,2 ; 3,1 ; 2"
   */
  static s4u::DragonflyParams parse_topo_parameters(const std::string& topo_parameters);

  /** @brief Checks topology parameters */
  static void check_topology(unsigned int n_groups, unsigned int groups_links, unsigned int n_chassis,
                             unsigned int chassis_links, unsigned int n_routers, unsigned int routers_links,
                             unsigned int nodes);
  /** @brief Set Dragonfly topology */
  void set_topology(unsigned int n_groups, unsigned int groups_links, unsigned int n_chassis,
                    unsigned int chassis_links, unsigned int n_routers, unsigned int routers_links, unsigned int nodes);
  /** @brief Build upper levels (routers) in Dragonfly */
  void build_upper_levels(const s4u::ClusterCallbacks& set_callbacks);
  /** @brief Set the characteristics of links inside the Dragonfly zone */
  void set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy) override;
  Coords rankId_to_coords(unsigned long rank_id) const;

private:
  void generate_routers(const s4u::ClusterCallbacks& set_callbacks);
  void generate_links();
  void generate_link(const std::string& id, int numlinks, resource::StandardLinkImpl** linkup,
                     resource::StandardLinkImpl** linkdown);

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
