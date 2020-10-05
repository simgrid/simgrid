/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_dragonfly, surf_route_cluster, "Dragonfly Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

DragonflyZone::DragonflyZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : ClusterZone(father, name, netmodel)
{
}

void DragonflyZone::rankId_to_coords(int rankId, unsigned int coords[4]) const
{
  // coords : group, chassis, blade, node
  coords[0] = rankId / (num_chassis_per_group_ * num_blades_per_chassis_ * num_nodes_per_blade_);
  rankId    = rankId % (num_chassis_per_group_ * num_blades_per_chassis_ * num_nodes_per_blade_);
  coords[1] = rankId / (num_blades_per_chassis_ * num_nodes_per_blade_);
  rankId    = rankId % (num_blades_per_chassis_ * num_nodes_per_blade_);
  coords[2] = rankId / num_nodes_per_blade_;
  coords[3] = rankId % num_nodes_per_blade_;
}

void DragonflyZone::parse_specific_arguments(ClusterCreationArgs* cluster)
{
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));

  if (parameters.size() != 4 || parameters.empty()) {
    surf_parse_error(
        "Dragonfly are defined by the number of groups, chassis per groups, blades per chassis, nodes per blade");
  }

  // Blue network : number of groups, number of links between each group
  boost::split(tmp, parameters[0], boost::is_any_of(","));
  if (tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  try {
    this->num_groups_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->num_links_blue_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the blue level:") + tmp[1]);
  }
  // Black network : number of chassis/group, number of links between each router on the black network
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if (tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  try {
    this->num_chassis_per_group_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->num_links_black_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the black level:") + tmp[1]);
  }

  // Green network : number of blades/chassis, number of links between each router on the green network
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if (tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  try {
    this->num_blades_per_chassis_ = std::stoi(tmp[0]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->num_links_green_ = std::stoi(tmp[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Invalid number of links for the green level:") + tmp[1]);
  }

  // The last part of topo_parameters should be the number of nodes per blade
  try {
    this->num_nodes_per_blade_ = std::stoi(parameters[3]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument(std::string("Last parameter is not the amount of nodes per blade:") + parameters[3]);
  }

  this->sharing_policy_ = cluster->sharing_policy;
  if (cluster->sharing_policy == s4u::Link::SharingPolicy::SPLITDUPLEX)
    this->num_links_per_link_ = 2;
  this->bw_  = cluster->bw;
  this->lat_ = cluster->lat;
}

/* Generate the cluster once every node is created */
void DragonflyZone::seal()
{
  if (this->num_nodes_per_blade_ == 0) {
    return;
  }

  this->generate_routers();
  this->generate_links();
}

void DragonflyZone::generate_routers()
{
  this->routers_.reserve(this->num_groups_ * this->num_chassis_per_group_ * this->num_blades_per_chassis_);
  for (unsigned int i = 0; i < this->num_groups_; i++)
    for (unsigned int j = 0; j < this->num_chassis_per_group_; j++)
      for (unsigned int k = 0; k < this->num_blades_per_chassis_; k++)
        this->routers_.emplace_back(i, j, k);
}

void DragonflyZone::generate_link(const std::string& id, int numlinks, resource::LinkImpl** linkup,
                                  resource::LinkImpl** linkdown) const
{
  *linkup   = nullptr;
  *linkdown = nullptr;
  LinkCreationArgs linkTemplate;
  linkTemplate.bandwidths.push_back(this->bw_ * numlinks);
  linkTemplate.latency   = this->lat_;
  linkTemplate.policy    = this->sharing_policy_;
  linkTemplate.id        = std::move(id);
  sg_platf_new_link(&linkTemplate);
  XBT_DEBUG("Generating link %s", linkTemplate.id.c_str());
  resource::LinkImpl* link;
  if (this->sharing_policy_ == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    *linkup   = s4u::Link::by_name(linkTemplate.id + "_UP")->get_impl();   // check link?
    *linkdown = s4u::Link::by_name(linkTemplate.id + "_DOWN")->get_impl(); // check link ?
  } else {
    link      = s4u::Link::by_name(linkTemplate.id)->get_impl();
    *linkup   = link;
    *linkdown = link;
  }
}

void DragonflyZone::generate_links()
{
  static int uniqueId = 0;
  resource::LinkImpl* linkup;
  resource::LinkImpl* linkdown;

  unsigned int numRouters = this->num_groups_ * this->num_chassis_per_group_ * this->num_blades_per_chassis_;

  // Links from routers to their local nodes.
  for (unsigned int i = 0; i < numRouters; i++) {
    // allocate structures
    this->routers_[i].my_nodes_.resize(num_links_per_link_ * this->num_nodes_per_blade_);
    this->routers_[i].green_links_.resize(this->num_blades_per_chassis_);
    this->routers_[i].black_links_.resize(this->num_chassis_per_group_);

    for (unsigned int j = 0; j < num_links_per_link_ * this->num_nodes_per_blade_; j += num_links_per_link_) {
      std::string id = "local_link_from_router_" + std::to_string(i) + "_to_node_" +
                       std::to_string(j / num_links_per_link_) + "_" + std::to_string(uniqueId);
      this->generate_link(id, 1, &linkup, &linkdown);

      this->routers_[i].my_nodes_[j] = linkup;
      if (this->sharing_policy_ == s4u::Link::SharingPolicy::SPLITDUPLEX)
        this->routers_[i].my_nodes_[j + 1] = linkdown;

      uniqueId++;
    }
  }

  // Green links from routers to same chassis routers - alltoall
  for (unsigned int i = 0; i < this->num_groups_ * this->num_chassis_per_group_; i++) {
    for (unsigned int j = 0; j < this->num_blades_per_chassis_; j++) {
      for (unsigned int k = j + 1; k < this->num_blades_per_chassis_; k++) {
        std::string id = "green_link_in_chassis_" + std::to_string(i % num_chassis_per_group_) + "_between_routers_" +
                         std::to_string(j) + "_and_" + std::to_string(k) + "_" + std::to_string(uniqueId);
        this->generate_link(id, this->num_links_green_, &linkup, &linkdown);

        this->routers_[i * num_blades_per_chassis_ + j].green_links_[k] = linkup;
        this->routers_[i * num_blades_per_chassis_ + k].green_links_[j] = linkdown;
        uniqueId++;
      }
    }
  }

  // Black links from routers to same group routers - alltoall
  for (unsigned int i = 0; i < this->num_groups_; i++) {
    for (unsigned int j = 0; j < this->num_chassis_per_group_; j++) {
      for (unsigned int k = j + 1; k < this->num_chassis_per_group_; k++) {
        for (unsigned int l = 0; l < this->num_blades_per_chassis_; l++) {
          std::string id = "black_link_in_group_" + std::to_string(i) + "_between_chassis_" + std::to_string(j) +
              "_and_" + std::to_string(k) +"_blade_" + std::to_string(l) + "_" + std::to_string(uniqueId);
          this->generate_link(id, this->num_links_black_, &linkup, &linkdown);

          this->routers_[i * num_blades_per_chassis_ * num_chassis_per_group_ + j * num_blades_per_chassis_ + l]
              .black_links_[k] = linkup;
          this->routers_[i * num_blades_per_chassis_ * num_chassis_per_group_ + k * num_blades_per_chassis_ + l]
              .black_links_[j] = linkdown;
          uniqueId++;
        }
      }
    }
  }

  // Blue links between groups - Not all routers involved, only one per group is linked to others. Let's say router n of
  // each group is linked to group n.
  // FIXME: in reality blue links may be attached to several different routers
  for (unsigned int i = 0; i < this->num_groups_; i++) {
    for (unsigned int j = i + 1; j < this->num_groups_; j++) {
      unsigned int routernumi                 = i * num_blades_per_chassis_ * num_chassis_per_group_ + j;
      unsigned int routernumj                 = j * num_blades_per_chassis_ * num_chassis_per_group_ + i;
      std::string id = "blue_link_between_group_"+ std::to_string(i) +"_and_" + std::to_string(j) +"_routers_" +
          std::to_string(routernumi) + "_and_" + std::to_string(routernumj) + "_" + std::to_string(uniqueId);
      this->generate_link(id, this->num_links_blue_, &linkup, &linkdown);

      this->routers_[routernumi].blue_link_ = linkup;
      this->routers_[routernumj].blue_link_ = linkdown;
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

  if ((src->id() == dst->id()) && has_loopback_) {
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos(src->id()));

    route->link_list.push_back(info.first);
    if (latency)
      *latency += info.first->get_latency();
    return;
  }

  unsigned int myCoords[4];
  rankId_to_coords(src->id(), myCoords);
  unsigned int targetCoords[4];
  rankId_to_coords(dst->id(), targetCoords);
  XBT_DEBUG("src : %u group, %u chassis, %u blade, %u node", myCoords[0], myCoords[1], myCoords[2], myCoords[3]);
  XBT_DEBUG("dst : %u group, %u chassis, %u blade, %u node", targetCoords[0], targetCoords[1], targetCoords[2],
            targetCoords[3]);

  DragonflyRouter* myRouter = &routers_[myCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                        myCoords[1] * num_blades_per_chassis_ + myCoords[2]];
  DragonflyRouter* targetRouter = &routers_[targetCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                            targetCoords[1] * num_blades_per_chassis_ + targetCoords[2]];
  DragonflyRouter* currentRouter = myRouter;

  // node->router local link
  route->link_list.push_back(myRouter->my_nodes_[myCoords[3] * num_links_per_link_]);
  if (latency)
    *latency += myRouter->my_nodes_[myCoords[3] * num_links_per_link_]->get_latency();

  if (has_limiter_) { // limiter for sender
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos_with_loopback(src->id()));
    route->link_list.push_back(info.first);
  }

  if (targetRouter != myRouter) {
    // are we on a different group ?
    if (targetRouter->group_ != currentRouter->group_) {
      // go to the router of our group connected to this one.
      if (currentRouter->blade_ != targetCoords[0]) {
        // go to the nth router in our chassis
        route->link_list.push_back(currentRouter->green_links_[targetCoords[0]]);
        if (latency)
          *latency += currentRouter->green_links_[targetCoords[0]]->get_latency();
        currentRouter = &routers_[myCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                  myCoords[1] * num_blades_per_chassis_ + targetCoords[0]];
      }

      if (currentRouter->chassis_ != 0) {
        // go to the first chassis of our group
        route->link_list.push_back(currentRouter->black_links_[0]);
        if (latency)
          *latency += currentRouter->black_links_[0]->get_latency();
        currentRouter = &routers_[myCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords[0]];
      }

      // go to destination group - the only optical hop
      route->link_list.push_back(currentRouter->blue_link_);
      if (latency)
        *latency += currentRouter->blue_link_->get_latency();
      currentRouter = &routers_[targetCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) + myCoords[0]];
    }

    // same group, but same blade ?
    if (targetRouter->blade_ != currentRouter->blade_) {
      route->link_list.push_back(currentRouter->green_links_[targetCoords[2]]);
      if (latency)
        *latency += currentRouter->green_links_[targetCoords[2]]->get_latency();
      currentRouter = &routers_[targetCoords[0] * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords[2]];
    }

    // same blade, but same chassis ?
    if (targetRouter->chassis_ != currentRouter->chassis_) {
      route->link_list.push_back(currentRouter->black_links_[targetCoords[1]]);
      if (latency)
        *latency += currentRouter->black_links_[targetCoords[1]]->get_latency();
    }
  }

  if (has_limiter_) { // limiter for receiver
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos_with_loopback(dst->id()));
    route->link_list.push_back(info.first);
  }

  // router->node local link
  route->link_list.push_back(targetRouter->my_nodes_[targetCoords[3] * num_links_per_link_ + num_links_per_link_ - 1]);
  if (latency)
    *latency += targetRouter->my_nodes_[targetCoords[3] * num_links_per_link_ + num_links_per_link_ - 1]->get_latency();
}
}
}
} // namespace
