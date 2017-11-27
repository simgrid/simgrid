/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/routing/DragonflyZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_dragonfly, surf_route_cluster, "Dragonfly Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

DragonflyZone::DragonflyZone(NetZone* father, std::string name) : ClusterZone(father, name)
{
}

DragonflyZone::~DragonflyZone()
{
  if (this->routers_ != nullptr) {
    for (unsigned int i = 0; i < this->numGroups_ * this->numChassisPerGroup_ * this->numBladesPerChassis_; i++)
      delete routers_[i];
    delete[] routers_;
  }
}

void DragonflyZone::rankId_to_coords(int rankId, unsigned int (*coords)[4])
{
  // coords : group, chassis, blade, node
  (*coords)[0]         = rankId / (numChassisPerGroup_ * numBladesPerChassis_ * numNodesPerBlade_);
  rankId               = rankId % (numChassisPerGroup_ * numBladesPerChassis_ * numNodesPerBlade_);
  (*coords)[1]         = rankId / (numBladesPerChassis_ * numNodesPerBlade_);
  rankId               = rankId % (numBladesPerChassis_ * numNodesPerBlade_);
  (*coords)[2]         = rankId / numNodesPerBlade_;
  (*coords)[3]         = rankId % numNodesPerBlade_;
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
    this->numGroups_ = std::stoi(tmp[0]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->numLinksBlue_ = std::stoi(tmp[1]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of links for the blue level:") + tmp[1]);
  }
  // Black network : number of chassis/group, number of links between each router on the black network
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if (tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  try {
    this->numChassisPerGroup_ = std::stoi(tmp[0]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->numLinksBlack_ = std::stoi(tmp[1]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of links for the black level:") + tmp[1]);
  }

  // Green network : number of blades/chassis, number of links between each router on the green network
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if (tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  try {
    this->numBladesPerChassis_ = std::stoi(tmp[0]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of groups:") + tmp[0]);
  }

  try {
    this->numLinksGreen_ = std::stoi(tmp[1]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Invalid number of links for the green level:") + tmp[1]);
  }

  // The last part of topo_parameters should be the number of nodes per blade
  try {
    this->numNodesPerBlade_ = std::stoi(parameters[3]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Last parameter is not the amount of nodes per blade:") + parameters[3]);
  }

  if (cluster->sharing_policy == SURF_LINK_FULLDUPLEX)
    this->numLinksperLink_ = 2;

  this->cluster_ = cluster;
}

/* Generate the cluster once every node is created */
void DragonflyZone::seal()
{
  if (this->numNodesPerBlade_ == 0) {
    return;
  }

  this->generateRouters();
  this->generateLinks();
}

DragonflyRouter::DragonflyRouter(int group, int chassis, int blade) : group_(group), chassis_(chassis), blade_(blade)
{
}

DragonflyRouter::~DragonflyRouter()
{
  delete[] myNodes_;
  delete[] greenLinks_;
  delete[] blackLinks_;
  delete blueLinks_;
}

void DragonflyZone::generateRouters()
{
  this->routers_ = new DragonflyRouter*[this->numGroups_ * this->numChassisPerGroup_ * this->numBladesPerChassis_];

  for (unsigned int i = 0; i < this->numGroups_; i++) {
    for (unsigned int j = 0; j < this->numChassisPerGroup_; j++) {
      for (unsigned int k = 0; k < this->numBladesPerChassis_; k++) {
        DragonflyRouter* router = new DragonflyRouter(i, j, k);
        this->routers_[i * this->numChassisPerGroup_ * this->numBladesPerChassis_ + j * this->numBladesPerChassis_ +
                       k] = router;
      }
    }
  }
}

void DragonflyZone::createLink(const std::string& id, int numlinks, surf::LinkImpl** linkup, surf::LinkImpl** linkdown)
{
  *linkup   = nullptr;
  *linkdown = nullptr;
  LinkCreationArgs linkTemplate;
  linkTemplate.bandwidth = this->cluster_->bw * numlinks;
  linkTemplate.latency   = this->cluster_->lat;
  linkTemplate.policy    = this->cluster_->sharing_policy; // sthg to do with that ?
  linkTemplate.id        = id;
  sg_platf_new_link(&linkTemplate);
  XBT_DEBUG("Generating link %s", id.c_str());
  surf::LinkImpl* link;
  if (this->cluster_->sharing_policy == SURF_LINK_FULLDUPLEX) {
    *linkup   = surf::LinkImpl::byName(linkTemplate.id + "_UP");   // check link?
    *linkdown = surf::LinkImpl::byName(linkTemplate.id + "_DOWN"); // check link ?
  } else {
    link      = surf::LinkImpl::byName(linkTemplate.id);
    *linkup   = link;
    *linkdown = link;
  }
}

void DragonflyZone::generateLinks()
{
  static int uniqueId = 0;
  surf::LinkImpl* linkup;
  surf::LinkImpl* linkdown;

  unsigned int numRouters = this->numGroups_ * this->numChassisPerGroup_ * this->numBladesPerChassis_;

  // Links from routers to their local nodes.
  for (unsigned int i = 0; i < numRouters; i++) {
    // allocate structures
    this->routers_[i]->myNodes_    = new surf::LinkImpl*[numLinksperLink_ * this->numNodesPerBlade_];
    this->routers_[i]->greenLinks_ = new surf::LinkImpl*[this->numBladesPerChassis_];
    this->routers_[i]->blackLinks_ = new surf::LinkImpl*[this->numChassisPerGroup_];

    for (unsigned int j = 0; j < numLinksperLink_ * this->numNodesPerBlade_; j += numLinksperLink_) {
      std::string id = "local_link_from_router_"+ std::to_string(i) + "_to_node_" +
          std::to_string(j / numLinksperLink_) + "_" + std::to_string(uniqueId);
      this->createLink(id, 1, &linkup, &linkdown);

      this->routers_[i]->myNodes_[j] = linkup;
      if (this->cluster_->sharing_policy == SURF_LINK_FULLDUPLEX)
        this->routers_[i]->myNodes_[j + 1] = linkdown;

      uniqueId++;
    }
  }

  // Green links from routers to same chassis routers - alltoall
  for (unsigned int i = 0; i < this->numGroups_ * this->numChassisPerGroup_; i++) {
    for (unsigned int j = 0; j < this->numBladesPerChassis_; j++) {
      for (unsigned int k = j + 1; k < this->numBladesPerChassis_; k++) {
        std::string id = "green_link_in_chassis_" + std::to_string(i % numChassisPerGroup_) +"_between_routers_" +
            std::to_string(j) + "_and_" + std::to_string(k) + "_" + std::to_string(uniqueId);
        this->createLink(id, this->numLinksGreen_, &linkup, &linkdown);

        this->routers_[i * numBladesPerChassis_ + j]->greenLinks_[k] = linkup;
        this->routers_[i * numBladesPerChassis_ + k]->greenLinks_[j] = linkdown;
        uniqueId++;
      }
    }
  }

  // Black links from routers to same group routers - alltoall
  for (unsigned int i = 0; i < this->numGroups_; i++) {
    for (unsigned int j = 0; j < this->numChassisPerGroup_; j++) {
      for (unsigned int k = j + 1; k < this->numChassisPerGroup_; k++) {
        for (unsigned int l = 0; l < this->numBladesPerChassis_; l++) {
          std::string id = "black_link_in_group_" + std::to_string(i) + "_between_chassis_" + std::to_string(j) +
              "_and_" + std::to_string(k) +"_blade_" + std::to_string(l) + "_" + std::to_string(uniqueId);
          this->createLink(id, this->numLinksBlack_, &linkup, &linkdown);

          this->routers_[i * numBladesPerChassis_ * numChassisPerGroup_ + j * numBladesPerChassis_ + l]
              ->blackLinks_[k] = linkup;
          this->routers_[i * numBladesPerChassis_ * numChassisPerGroup_ + k * numBladesPerChassis_ + l]
              ->blackLinks_[j] = linkdown;
          uniqueId++;
        }
      }
    }
  }

  // Blue links between groups - Not all routers involved, only one per group is linked to others. Let's say router n of
  // each group is linked to group n.
  // FIXME: in reality blue links may be attached to several different routers
  for (unsigned int i = 0; i < this->numGroups_; i++) {
    for (unsigned int j = i + 1; j < this->numGroups_; j++) {
      unsigned int routernumi                = i * numBladesPerChassis_ * numChassisPerGroup_ + j;
      unsigned int routernumj                = j * numBladesPerChassis_ * numChassisPerGroup_ + i;
      this->routers_[routernumi]->blueLinks_ = new surf::LinkImpl*;
      this->routers_[routernumj]->blueLinks_ = new surf::LinkImpl*;
      std::string id = "blue_link_between_group_"+ std::to_string(i) +"_and_" + std::to_string(j) +"_routers_" +
          std::to_string(routernumi) + "_and_" + std::to_string(routernumj) + "_" + std::to_string(uniqueId);
      this->createLink(id, this->numLinksBlue_, &linkup, &linkdown);

      this->routers_[routernumi]->blueLinks_[0] = linkup;
      this->routers_[routernumj]->blueLinks_[0] = linkdown;
      uniqueId++;
    }
  }
}

void DragonflyZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t route, double* latency)
{
  // Minimal routing version.
  // TODO : non-minimal random one, and adaptive ?

  if (dst->isRouter() || src->isRouter())
    return;

  XBT_VERB("dragonfly getLocalRoute from '%s'[%u] to '%s'[%u]", src->getCname(), src->id(), dst->getCname(), dst->id());

  if ((src->id() == dst->id()) && hasLoopback_) {
    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePosition(src->id()));

    route->link_list.push_back(info.first);
    if (latency)
      *latency += info.first->latency();
    return;
  }

  unsigned int myCoords[4];
  rankId_to_coords(src->id(), &myCoords);
  unsigned int targetCoords[4];
  rankId_to_coords(dst->id(), &targetCoords);
  XBT_DEBUG("src : %u group, %u chassis, %u blade, %u node", myCoords[0], myCoords[1], myCoords[2], myCoords[3]);
  XBT_DEBUG("dst : %u group, %u chassis, %u blade, %u node", targetCoords[0], targetCoords[1], targetCoords[2],
            targetCoords[3]);

  DragonflyRouter* myRouter = routers_[myCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) +
                                       myCoords[1] * numBladesPerChassis_ + myCoords[2]];
  DragonflyRouter* targetRouter = routers_[targetCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) +
                                           targetCoords[1] * numBladesPerChassis_ + targetCoords[2]];
  DragonflyRouter* currentRouter = myRouter;

  // node->router local link
  route->link_list.push_back(myRouter->myNodes_[myCoords[3] * numLinksperLink_]);
  if (latency)
    *latency += myRouter->myNodes_[myCoords[3] * numLinksperLink_]->latency();

  if (hasLimiter_) { // limiter for sender
    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePositionWithLoopback(src->id()));
    route->link_list.push_back(info.first);
  }

  if (targetRouter != myRouter) {

    // are we on a different group ?
    if (targetRouter->group_ != currentRouter->group_) {
      // go to the router of our group connected to this one.
      if (currentRouter->blade_ != targetCoords[0]) {
        // go to the nth router in our chassis
        route->link_list.push_back(currentRouter->greenLinks_[targetCoords[0]]);
        if (latency)
          *latency += currentRouter->greenLinks_[targetCoords[0]]->latency();
        currentRouter = routers_[myCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) +
                                 myCoords[1] * numBladesPerChassis_ + targetCoords[0]];
      }

      if (currentRouter->chassis_ != 0) {
        // go to the first chassis of our group
        route->link_list.push_back(currentRouter->blackLinks_[0]);
        if (latency)
          *latency += currentRouter->blackLinks_[0]->latency();
        currentRouter = routers_[myCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) + targetCoords[0]];
      }

      // go to destination group - the only optical hop
      route->link_list.push_back(currentRouter->blueLinks_[0]);
      if (latency)
        *latency += currentRouter->blueLinks_[0]->latency();
      currentRouter = routers_[targetCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) + myCoords[0]];
    }

    // same group, but same blade ?
    if (targetRouter->blade_ != currentRouter->blade_) {
      route->link_list.push_back(currentRouter->greenLinks_[targetCoords[2]]);
      if (latency)
        *latency += currentRouter->greenLinks_[targetCoords[2]]->latency();
      currentRouter = routers_[targetCoords[0] * (numChassisPerGroup_ * numBladesPerChassis_) + targetCoords[2]];
    }

    // same blade, but same chassis ?
    if (targetRouter->chassis_ != currentRouter->chassis_) {
      route->link_list.push_back(currentRouter->blackLinks_[targetCoords[1]]);
      if (latency)
        *latency += currentRouter->blackLinks_[targetCoords[1]]->latency();
    }
  }

  if (hasLimiter_) { // limiter for receiver
    std::pair<surf::LinkImpl*, surf::LinkImpl*> info = privateLinks_.at(nodePositionWithLoopback(dst->id()));
    route->link_list.push_back(info.first);
  }

  // router->node local link
  route->link_list.push_back(targetRouter->myNodes_[targetCoords[3] * numLinksperLink_ + numLinksperLink_ - 1]);
  if (latency)
    *latency += targetRouter->myNodes_[targetCoords[3] * numLinksperLink_ + numLinksperLink_ - 1]->latency();
}
}
}
} // namespace
