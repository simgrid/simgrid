/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <numeric>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_dragonfly, surf_route_cluster, "Dragonfly Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

DragonflyZone::DragonflyZone(const std::string& name) : ClusterZone(name) {}

DragonflyZone::Coords DragonflyZone::rankId_to_coords(int rankId) const
{
  // coords : group, chassis, blade, node
  Coords coords;
  coords.group   = rankId / (num_chassis_per_group_ * num_blades_per_chassis_ * num_nodes_per_blade_);
  rankId         = rankId % (num_chassis_per_group_ * num_blades_per_chassis_ * num_nodes_per_blade_);
  coords.chassis = rankId / (num_blades_per_chassis_ * num_nodes_per_blade_);
  rankId         = rankId % (num_blades_per_chassis_ * num_nodes_per_blade_);
  coords.blade   = rankId / num_nodes_per_blade_;
  coords.node    = rankId % num_nodes_per_blade_;
  return coords;
}

void DragonflyZone::rankId_to_coords(int rankId, unsigned int coords[4]) const // XBT_ATTRIB_DEPRECATED_v330
{
  const auto s_coords = rankId_to_coords(rankId);
  coords[0]           = s_coords.group;
  coords[1]           = s_coords.chassis;
  coords[2]           = s_coords.blade;
  coords[3]           = s_coords.node;
}

void DragonflyZone::set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy)
{
  sharing_policy_ = sharing_policy;
  if (sharing_policy == s4u::Link::SharingPolicy::SPLITDUPLEX)
    num_links_per_link_ = 2;
  bw_  = bw;
  lat_ = lat;
}

void DragonflyZone::set_topology(unsigned int n_groups, unsigned int groups_links, unsigned int n_chassis,
                                 unsigned int chassis_links, unsigned int n_routers, unsigned int routers_links,
                                 unsigned int nodes)
{
  num_groups_     = n_groups;
  num_links_blue_ = groups_links;

  num_chassis_per_group_ = n_chassis;
  num_links_black_       = chassis_links;

  num_blades_per_chassis_ = n_routers;
  num_links_green_        = routers_links;

  num_nodes_per_blade_ = nodes;
}

void DragonflyZone::parse_specific_arguments(ClusterCreationArgs* cluster)
{
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));

  if (parameters.size() != 4)
    surf_parse_error(
        "Dragonfly are defined by the number of groups, chassis per groups, blades per chassis, nodes per blade");

  // Blue network : number of groups, number of links between each group
  boost::split(tmp, parameters[0], boost::is_any_of(","));
  if (tmp.size() != 2)
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");

  try {
    num_groups_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    num_links_blue_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the blue level:") + tmp[1]);
  }

  // Black network : number of chassis/group, number of links between each router on the black network
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if (tmp.size() != 2)
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");

  try {
    num_chassis_per_group_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    num_links_black_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the black level:") + tmp[1]);
  }

  // Green network : number of blades/chassis, number of links between each router on the green network
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if (tmp.size() != 2)
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");

  try {
    num_blades_per_chassis_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    num_links_green_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the green level:") + tmp[1]);
  }

  // The last part of topo_parameters should be the number of nodes per blade
  try {
    num_nodes_per_blade_ = std::stoi(parameters[3]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Last parameter is not the amount of nodes per blade:") + parameters[3]);
  }

  set_link_characteristics(cluster->bw, cluster->lat, cluster->sharing_policy);
}

/* Generate the cluster once every node is created */
void DragonflyZone::do_seal()
{
  if (num_nodes_per_blade_ == 0)
    return;

  generate_routers();
  generate_links();
}

void DragonflyZone::generate_routers()
{
  routers_.reserve(num_groups_ * num_chassis_per_group_ * num_blades_per_chassis_);
  for (unsigned int i = 0; i < num_groups_; i++)
    for (unsigned int j = 0; j < num_chassis_per_group_; j++)
      for (unsigned int k = 0; k < num_blades_per_chassis_; k++)
        routers_.emplace_back(i, j, k);
}

void DragonflyZone::generate_link(const std::string& id, int numlinks, resource::LinkImpl** linkup,
                                  resource::LinkImpl** linkdown)
{
  XBT_DEBUG("Generating link %s", id.c_str());
  *linkup   = nullptr;
  *linkdown = nullptr;
  if (sharing_policy_ == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    *linkup   = create_link(id + "_UP", std::vector<double>{bw_ * numlinks})->set_latency(lat_)->seal()->get_impl();
    *linkdown = create_link(id + "_DOWN", std::vector<double>{bw_ * numlinks})->set_latency(lat_)->seal()->get_impl();
  } else {
    *linkup   = create_link(id, std::vector<double>{bw_ * numlinks})->set_latency(lat_)->seal()->get_impl();
    *linkdown = *linkup;
  }
}

void DragonflyZone::generate_links()
{
  static int uniqueId = 0;
  resource::LinkImpl* linkup;
  resource::LinkImpl* linkdown;

  unsigned int numRouters = num_groups_ * num_chassis_per_group_ * num_blades_per_chassis_;

  // Links from routers to their local nodes.
  for (unsigned int i = 0; i < numRouters; i++) {
    // allocate structures
    routers_[i].my_nodes_.resize(num_links_per_link_ * num_nodes_per_blade_);
    routers_[i].green_links_.resize(num_blades_per_chassis_);
    routers_[i].black_links_.resize(num_chassis_per_group_);

    for (unsigned int j = 0; j < num_links_per_link_ * num_nodes_per_blade_; j += num_links_per_link_) {
      std::string id = "local_link_from_router_" + std::to_string(i) + "_to_node_" +
                       std::to_string(j / num_links_per_link_) + "_" + std::to_string(uniqueId);
      generate_link(id, 1, &linkup, &linkdown);

      routers_[i].my_nodes_[j] = linkup;
      if (sharing_policy_ == s4u::Link::SharingPolicy::SPLITDUPLEX)
        routers_[i].my_nodes_[j + 1] = linkdown;

      uniqueId++;
    }
  }

  // Green links from routers to same chassis routers - alltoall
  for (unsigned int i = 0; i < num_groups_ * num_chassis_per_group_; i++) {
    for (unsigned int j = 0; j < num_blades_per_chassis_; j++) {
      for (unsigned int k = j + 1; k < num_blades_per_chassis_; k++) {
        std::string id = "green_link_in_chassis_" + std::to_string(i % num_chassis_per_group_) + "_between_routers_" +
                         std::to_string(j) + "_and_" + std::to_string(k) + "_" + std::to_string(uniqueId);
        generate_link(id, num_links_green_, &linkup, &linkdown);

        routers_[i * num_blades_per_chassis_ + j].green_links_[k] = linkup;
        routers_[i * num_blades_per_chassis_ + k].green_links_[j] = linkdown;
        uniqueId++;
      }
    }
  }

  // Black links from routers to same group routers - alltoall
  for (unsigned int i = 0; i < num_groups_; i++) {
    for (unsigned int j = 0; j < num_chassis_per_group_; j++) {
      for (unsigned int k = j + 1; k < num_chassis_per_group_; k++) {
        for (unsigned int l = 0; l < num_blades_per_chassis_; l++) {
          std::string id = "black_link_in_group_" + std::to_string(i) + "_between_chassis_" + std::to_string(j) +
                           "_and_" + std::to_string(k) + "_blade_" + std::to_string(l) + "_" + std::to_string(uniqueId);
          generate_link(id, num_links_black_, &linkup, &linkdown);

          routers_[i * num_blades_per_chassis_ * num_chassis_per_group_ + j * num_blades_per_chassis_ + l]
              .black_links_[k] = linkup;
          routers_[i * num_blades_per_chassis_ * num_chassis_per_group_ + k * num_blades_per_chassis_ + l]
              .black_links_[j] = linkdown;
          uniqueId++;
        }
      }
    }
  }

  // Blue links between groups - Not all routers involved, only one per group is linked to others. Let's say router n of
  // each group is linked to group n.
  // FIXME: in reality blue links may be attached to several different routers
  for (unsigned int i = 0; i < num_groups_; i++) {
    for (unsigned int j = i + 1; j < num_groups_; j++) {
      unsigned int routernumi = i * num_blades_per_chassis_ * num_chassis_per_group_ + j;
      unsigned int routernumj = j * num_blades_per_chassis_ * num_chassis_per_group_ + i;
      std::string id = "blue_link_between_group_" + std::to_string(i) + "_and_" + std::to_string(j) + "_routers_" +
                       std::to_string(routernumi) + "_and_" + std::to_string(routernumj) + "_" +
                       std::to_string(uniqueId);
      generate_link(id, num_links_blue_, &linkup, &linkdown);

      routers_[routernumi].blue_link_ = linkup;
      routers_[routernumj].blue_link_ = linkdown;
      uniqueId++;
    }
  }
}

void DragonflyZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* latency)
{
  // Minimal routing version.
  // TODO : non-minimal random one, and adaptive ?

  if (dst->is_router() || src->is_router())
    return;

  XBT_VERB("dragonfly getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(),
           dst->id());

  if ((src->id() == dst->id()) && has_loopback()) {
    resource::LinkImpl* uplink = get_uplink_from(node_pos(src->id()));

    route->link_list.push_back(uplink);
    if (latency)
      *latency += uplink->get_latency();
    return;
  }

  const auto myCoords     = rankId_to_coords(src->id());
  const auto targetCoords = rankId_to_coords(dst->id());
  XBT_DEBUG("src : %u group, %u chassis, %u blade, %u node", myCoords.group, myCoords.chassis, myCoords.blade,
            myCoords.node);
  XBT_DEBUG("dst : %u group, %u chassis, %u blade, %u node", targetCoords.group, targetCoords.chassis,
            targetCoords.blade, targetCoords.node);

  DragonflyRouter* myRouter      = &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                        myCoords.chassis * num_blades_per_chassis_ + myCoords.blade];
  DragonflyRouter* targetRouter  = &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                            targetCoords.chassis * num_blades_per_chassis_ + targetCoords.blade];
  DragonflyRouter* currentRouter = myRouter;

  // node->router local link
  route->link_list.push_back(myRouter->my_nodes_[myCoords.node * num_links_per_link_]);
  if (latency)
    *latency += myRouter->my_nodes_[myCoords.node * num_links_per_link_]->get_latency();

  if (has_limiter()) { // limiter for sender
    route->link_list.push_back(get_uplink_from(node_pos_with_loopback(src->id())));
  }

  if (targetRouter != myRouter) {
    // are we on a different group ?
    if (targetRouter->group_ != currentRouter->group_) {
      // go to the router of our group connected to this one.
      if (currentRouter->blade_ != targetCoords.group) {
        // go to the nth router in our chassis
        route->link_list.push_back(currentRouter->green_links_[targetCoords.group]);
        if (latency)
          *latency += currentRouter->green_links_[targetCoords.group]->get_latency();
        currentRouter = &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                  myCoords.chassis * num_blades_per_chassis_ + targetCoords.group];
      }

      if (currentRouter->chassis_ != 0) {
        // go to the first chassis of our group
        route->link_list.push_back(currentRouter->black_links_[0]);
        if (latency)
          *latency += currentRouter->black_links_[0]->get_latency();
        currentRouter =
            &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords.group];
      }

      // go to destination group - the only optical hop
      route->link_list.push_back(currentRouter->blue_link_);
      if (latency)
        *latency += currentRouter->blue_link_->get_latency();
      currentRouter =
          &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + myCoords.group];
    }

    // same group, but same blade ?
    if (targetRouter->blade_ != currentRouter->blade_) {
      route->link_list.push_back(currentRouter->green_links_[targetCoords.blade]);
      if (latency)
        *latency += currentRouter->green_links_[targetCoords.blade]->get_latency();
      currentRouter =
          &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords.blade];
    }

    // same blade, but same chassis ?
    if (targetRouter->chassis_ != currentRouter->chassis_) {
      route->link_list.push_back(currentRouter->black_links_[targetCoords.chassis]);
      if (latency)
        *latency += currentRouter->black_links_[targetCoords.chassis]->get_latency();
    }
  }

  if (has_limiter()) { // limiter for receiver
    route->link_list.push_back(get_downlink_to(node_pos_with_loopback(dst->id())));
  }

  // router->node local link
  route->link_list.push_back(
      targetRouter->my_nodes_[targetCoords.node * num_links_per_link_ + num_links_per_link_ - 1]);
  if (latency)
    *latency +=
        targetRouter->my_nodes_[targetCoords.node * num_links_per_link_ + num_links_per_link_ - 1]->get_latency();

  // set gateways (if any)
  route->gw_src = get_gateway(src->id());
  route->gw_dst = get_gateway(dst->id());
}
} // namespace routing
} // namespace kernel

namespace s4u {
DragonflyParams::DragonflyParams(const std::pair<unsigned int, unsigned int>& groups,
                                 const std::pair<unsigned int, unsigned int>& chassis,
                                 const std::pair<unsigned int, unsigned int>& routers, unsigned int nodes)
    : groups(groups), chassis(chassis), routers(routers), nodes(nodes)
{
  if (groups.first == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of groups, must be > 0");
  if (groups.second == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of blue (groups) links, must be > 0");
  if (chassis.first == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of chassis, must be > 0");
  if (chassis.second == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of black (chassis) links, must be > 0");
  if (routers.first == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of routers, must be > 0");
  if (routers.second == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of green (routers) links, must be > 0");
  if (nodes == 0)
    throw std::invalid_argument("Dragonfly: Invalid number of nodes, must be > 0");
}

NetZone* create_dragonfly_zone(const std::string& name, const NetZone* parent, const DragonflyParams& params,
                               double bandwidth, double latency, Link::SharingPolicy sharing_policy,
                               const std::function<ClusterNetPointCb>& set_netpoint,
                               const std::function<ClusterLinkCb>& set_loopback,
                               const std::function<ClusterLinkCb>& set_limiter)
{
  /* initial checks */
  if (bandwidth <= 0)
    throw std::invalid_argument("DragonflyZone: incorrect bandwidth for internode communication, bw=" +
                                std::to_string(bandwidth));
  if (latency < 0)
    throw std::invalid_argument("DragonflyZone: incorrect latency for internode communication, lat=" +
                                std::to_string(latency));

  /* creating zone */
  auto* zone = new kernel::routing::DragonflyZone(name);
  zone->set_topology(params.groups.first, params.groups.second, params.chassis.first, params.chassis.second,
                     params.routers.first, params.routers.second, params.nodes);
  if (parent)
    zone->set_parent(parent->get_impl());
  zone->set_link_characteristics(bandwidth, latency, sharing_policy);

  /* populating it */
  std::vector<unsigned int> dimensions = {params.groups.first, params.chassis.first, params.routers.first,
                                          params.nodes};
  int tot_elements                     = std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<>());
  for (int i = 0; i < tot_elements; i++) {
    kernel::routing::NetPoint* netpoint;
    Link* limiter;
    Link* loopback;
    zone->fill_leaf_from_cb(i, dimensions, set_netpoint, set_loopback, set_limiter, &netpoint, &loopback, &limiter);
  }

  return zone->get_iface();
}
} // namespace s4u

} // namespace simgrid
