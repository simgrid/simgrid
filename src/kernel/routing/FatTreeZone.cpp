/* Copyright (c) 2014-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <fstream>
#include <sstream>
#include <string>

#include "src/kernel/routing/FatTreeZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_fat_tree, surf, "Routing for fat trees");

namespace simgrid {
namespace kernel {
namespace routing {

FatTreeZone::FatTreeZone(NetZone* father, std::string name) : ClusterZone(father, name)
{
  XBT_DEBUG("Creating a new fat tree.");
}

FatTreeZone::~FatTreeZone()
{
  for (unsigned int i = 0; i < this->nodes_.size(); i++) {
    delete this->nodes_[i];
  }
  for (unsigned int i = 0; i < this->links_.size(); i++) {
    delete this->links_[i];
  }
}

bool FatTreeZone::isInSubTree(FatTreeNode* root, FatTreeNode* node)
{
  XBT_DEBUG("Is %d(%u,%u) in the sub tree of %d(%u,%u) ?", node->id, node->level, node->position, root->id, root->level,
            root->position);
  if (root->level <= node->level) {
    return false;
  }
  for (unsigned int i = 0; i < node->level; i++) {
    if (root->label[i] != node->label[i]) {
      return false;
    }
  }

  for (unsigned int i = root->level; i < this->levels_; i++) {
    if (root->label[i] != node->label[i]) {
      return false;
    }
  }
  return true;
}

void FatTreeZone::getLocalRoute(NetPoint* src, NetPoint* dst, sg_platf_route_cbarg_t into, double* latency)
{

  if (dst->isRouter() || src->isRouter())
    return;

  /* Let's find the source and the destination in our internal structure */
  auto searchedNode = this->computeNodes_.find(src->id());
  xbt_assert(searchedNode != this->computeNodes_.end(), "Could not find the source %s [%u] in the fat tree",
             src->getCname(), src->id());
  FatTreeNode* source = searchedNode->second;

  searchedNode = this->computeNodes_.find(dst->id());
  xbt_assert(searchedNode != this->computeNodes_.end(), "Could not find the destination %s [%u] in the fat tree",
             dst->getCname(), dst->id());
  FatTreeNode* destination = searchedNode->second;

  XBT_VERB("Get route and latency from '%s' [%u] to '%s' [%u] in a fat tree", src->getCname(), src->id(),
           dst->getCname(), dst->id());

  /* In case destination is the source, and there is a loopback, let's use it instead of going up to a switch */
  if (source->id == destination->id && this->hasLoopback_) {
    into->link_list.push_back(source->loopback);
    if (latency)
      *latency += source->loopback->latency();
    return;
  }

  FatTreeNode* currentNode = source;

  // up part
  while (not isInSubTree(currentNode, destination)) {
    int d = destination->position; // as in d-mod-k

    for (unsigned int i = 0; i < currentNode->level; i++)
      d /= this->upperLevelNodesNumber_[i];

    int k = this->upperLevelNodesNumber_[currentNode->level];
    d     = d % k;
    into->link_list.push_back(currentNode->parents[d]->upLink);

    if (latency)
      *latency += currentNode->parents[d]->upLink->latency();

    if (this->hasLimiter_)
      into->link_list.push_back(currentNode->limiterLink);
    currentNode = currentNode->parents[d]->upNode;
  }

  XBT_DEBUG("%d(%u,%u) is in the sub tree of %d(%u,%u).", destination->id, destination->level, destination->position,
            currentNode->id, currentNode->level, currentNode->position);

  // Down part
  while (currentNode != destination) {
    for (unsigned int i = 0; i < currentNode->children.size(); i++) {
      if (i % this->lowerLevelNodesNumber_[currentNode->level - 1] == destination->label[currentNode->level - 1]) {
        into->link_list.push_back(currentNode->children[i]->downLink);
        if (latency)
          *latency += currentNode->children[i]->downLink->latency();
        currentNode = currentNode->children[i]->downNode;
        if (this->hasLimiter_)
          into->link_list.push_back(currentNode->limiterLink);
        XBT_DEBUG("%d(%u,%u) is accessible through %d(%u,%u)", destination->id, destination->level,
                  destination->position, currentNode->id, currentNode->level, currentNode->position);
      }
    }
  }
}

/* This function makes the assumption that parse_specific_arguments() and
 * addNodes() have already been called
 */
void FatTreeZone::seal()
{
  if (this->levels_ == 0) {
    return;
  }
  this->generateSwitches();

  if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;

    msgBuffer << "We are creating a fat tree of " << this->levels_ << " levels "
              << "with " << this->nodesByLevel_[0] << " processing nodes";
    for (unsigned int i = 1; i <= this->levels_; i++) {
      msgBuffer << ", " << this->nodesByLevel_[i] << " switches at level " << i;
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
    msgBuffer.str("");
    msgBuffer << "Nodes are : ";

    for (unsigned int i = 0; i < this->nodes_.size(); i++) {
      msgBuffer << this->nodes_[i]->id << "(" << this->nodes_[i]->level << "," << this->nodes_[i]->position << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }

  this->generateLabels();

  unsigned int k = 0;
  // Nodes are totally ordered, by level and then by position, in this->nodes
  for (unsigned int i = 0; i < this->levels_; i++) {
    for (unsigned int j = 0; j < this->nodesByLevel_[i]; j++) {
      this->connectNodeToParents(this->nodes_[k]);
      k++;
    }
  }

  if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;
    msgBuffer << "Links are : ";
    for (unsigned int i = 0; i < this->links_.size(); i++) {
      msgBuffer << "(" << this->links_[i]->upNode->id << "," << this->links_[i]->downNode->id << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }
}

int FatTreeZone::connectNodeToParents(FatTreeNode* node)
{
  std::vector<FatTreeNode*>::iterator currentParentNode = this->nodes_.begin();
  int connectionsNumber                                 = 0;
  const int level                                       = node->level;
  XBT_DEBUG("We are connecting node %d(%u,%u) to his parents.", node->id, node->level, node->position);
  currentParentNode += this->getLevelPosition(level + 1);
  for (unsigned int i = 0; i < this->nodesByLevel_[level + 1]; i++) {
    if (this->areRelated(*currentParentNode, node)) {
      XBT_DEBUG("%d(%u,%u) and %d(%u,%u) are related,"
                " with %u links between them.",
                node->id, node->level, node->position, (*currentParentNode)->id, (*currentParentNode)->level,
                (*currentParentNode)->position, this->lowerLevelPortsNumber_[level]);
      for (unsigned int j = 0; j < this->lowerLevelPortsNumber_[level]; j++) {
        this->addLink(*currentParentNode, node->label[level] + j * this->lowerLevelNodesNumber_[level], node,
                      (*currentParentNode)->label[level] + j * this->upperLevelNodesNumber_[level]);
      }
      connectionsNumber++;
    }
    ++currentParentNode;
  }
  return connectionsNumber;
}

bool FatTreeZone::areRelated(FatTreeNode* parent, FatTreeNode* child)
{
  std::stringstream msgBuffer;

  if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    msgBuffer << "Are " << child->id << "(" << child->level << "," << child->position << ") <";

    for (unsigned int i = 0; i < this->levels_; i++) {
      msgBuffer << child->label[i] << ",";
    }
    msgBuffer << ">";

    msgBuffer << " and " << parent->id << "(" << parent->level << "," << parent->position << ") <";
    for (unsigned int i = 0; i < this->levels_; i++) {
      msgBuffer << parent->label[i] << ",";
    }
    msgBuffer << ">";
    msgBuffer << " related ? ";
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }
  if (parent->level != child->level + 1) {
    return false;
  }

  for (unsigned int i = 0; i < this->levels_; i++) {
    if (parent->label[i] != child->label[i] && i + 1 != parent->level) {
      return false;
    }
  }
  return true;
}

void FatTreeZone::generateSwitches()
{
  XBT_DEBUG("Generating switches.");
  this->nodesByLevel_.resize(this->levels_ + 1, 0);

  // Take care of the number of nodes by level
  this->nodesByLevel_[0] = 1;
  for (unsigned int i = 0; i < this->levels_; i++)
    this->nodesByLevel_[0] *= this->lowerLevelNodesNumber_[i];

  if (this->nodesByLevel_[0] != this->nodes_.size()) {
    surf_parse_error(std::string("The number of provided nodes does not fit with the wanted topology.") +
                     " Please check your platform description (We need " + std::to_string(this->nodesByLevel_[0]) +
                     "nodes, we got " + std::to_string(this->nodes_.size()));
    return;
  }

  for (unsigned int i = 0; i < this->levels_; i++) {
    int nodesInThisLevel = 1;

    for (unsigned int j = 0; j <= i; j++)
      nodesInThisLevel *= this->upperLevelNodesNumber_[j];

    for (unsigned int j = i + 1; j < this->levels_; j++)
      nodesInThisLevel *= this->lowerLevelNodesNumber_[j];

    this->nodesByLevel_[i + 1] = nodesInThisLevel;
  }

  // Create the switches
  int k = 0;
  for (unsigned int i = 0; i < this->levels_; i++) {
    for (unsigned int j = 0; j < this->nodesByLevel_[i + 1]; j++) {
      FatTreeNode* newNode = new FatTreeNode(this->cluster_, --k, i + 1, j);
      XBT_DEBUG("We create the switch %d(%u,%u)", newNode->id, newNode->level, newNode->position);
      newNode->children.resize(this->lowerLevelNodesNumber_[i] * this->lowerLevelPortsNumber_[i]);
      if (i != this->levels_ - 1) {
        newNode->parents.resize(this->upperLevelNodesNumber_[i + 1] * this->lowerLevelPortsNumber_[i + 1]);
      }
      newNode->label.resize(this->levels_);
      this->nodes_.push_back(newNode);
    }
  }
}

void FatTreeZone::generateLabels()
{
  XBT_DEBUG("Generating labels.");
  // TODO : check if nodesByLevel and nodes are filled
  std::vector<int> maxLabel(this->levels_);
  std::vector<int> currentLabel(this->levels_);
  unsigned int k = 0;
  for (unsigned int i = 0; i <= this->levels_; i++) {
    currentLabel.assign(this->levels_, 0);
    for (unsigned int j = 0; j < this->levels_; j++) {
      maxLabel[j] = j + 1 > i ? this->lowerLevelNodesNumber_[j] : this->upperLevelNodesNumber_[j];
    }

    for (unsigned int j = 0; j < this->nodesByLevel_[i]; j++) {

      if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
        std::stringstream msgBuffer;

        msgBuffer << "Assigning label <";
        for (unsigned int l = 0; l < this->levels_; l++) {
          msgBuffer << currentLabel[l] << ",";
        }
        msgBuffer << "> to " << k << " (" << i << "," << j << ")";

        XBT_DEBUG("%s", msgBuffer.str().c_str());
      }
      this->nodes_[k]->label.assign(currentLabel.begin(), currentLabel.end());

      bool remainder   = true;
      unsigned int pos = 0;
      while (remainder && pos < this->levels_) {
        ++currentLabel[pos];
        if (currentLabel[pos] >= maxLabel[pos]) {
          currentLabel[pos] = 0;
          remainder         = true;
          ++pos;
        } else {
          pos       = 0;
          remainder = false;
        }
      }
      k++;
    }
  }
}

int FatTreeZone::getLevelPosition(const unsigned int level)
{
  xbt_assert(level <= this->levels_, "The impossible did happen. Yet again.");
  int tempPosition = 0;

  for (unsigned int i = 0; i < level; i++)
    tempPosition += this->nodesByLevel_[i];

  return tempPosition;
}

void FatTreeZone::addProcessingNode(int id)
{
  using std::make_pair;
  static int position = 0;
  FatTreeNode* newNode;
  newNode = new FatTreeNode(this->cluster_, id, 0, position++);
  newNode->parents.resize(this->upperLevelNodesNumber_[0] * this->lowerLevelPortsNumber_[0]);
  newNode->label.resize(this->levels_);
  this->computeNodes_.insert(make_pair(id, newNode));
  this->nodes_.push_back(newNode);
}

void FatTreeZone::addLink(FatTreeNode* parent, unsigned int parentPort, FatTreeNode* child, unsigned int childPort)
{
  FatTreeLink* newLink;
  newLink = new FatTreeLink(this->cluster_, child, parent);
  XBT_DEBUG("Creating a link between the parent (%u,%u,%u) and the child (%u,%u,%u)", parent->level, parent->position,
            parentPort, child->level, child->position, childPort);
  parent->children[parentPort] = newLink;
  child->parents[childPort]    = newLink;

  this->links_.push_back(newLink);
}

void FatTreeZone::parse_specific_arguments(ClusterCreationArgs* cluster)
{
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));
  const std::string error_msg {"Fat trees are defined by the levels number and 3 vectors, see the documentation for more information"};

  // TODO : we have to check for zeros and negative numbers, or it might crash
  if (parameters.size() != 4) {
    surf_parse_error(error_msg);
  }

  // The first parts of topo_parameters should be the levels number
  try {
    this->levels_ = std::stoi(parameters[0]);
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("First parameter is not the amount of levels:") + parameters[0]);
  }

  // Then, a l-sized vector standing for the children number by level
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if (tmp.size() != this->levels_) {
    surf_parse_error(error_msg);
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    try {
      this->lowerLevelNodesNumber_.push_back(std::stoi(tmp[i]));
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid lower level node number:") + tmp[i]);
    }
  }

  // Then, a l-sized vector standing for the parents number by level
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if (tmp.size() != this->levels_) {
    surf_parse_error(error_msg);
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    try {
      this->upperLevelNodesNumber_.push_back(std::stoi(tmp[i]));
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid upper level node number:") + tmp[i]);
    }
  }

  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  if (tmp.size() != this->levels_) {
    surf_parse_error(error_msg);
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    try {
      this->lowerLevelPortsNumber_.push_back(std::stoi(tmp[i]));
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid lower level port number:") + tmp[i]);
    }
  }
  this->cluster_ = cluster;
}

void FatTreeZone::generateDotFile(const std::string& filename) const
{
  std::ofstream file;
  file.open(filename, std::ios::out | std::ios::trunc);
  xbt_assert(file.is_open(), "Unable to open file %s", filename.c_str());

  file << "graph AsClusterFatTree {\n";
  for (unsigned int i = 0; i < this->nodes_.size(); i++) {
    file << this->nodes_[i]->id;
    if (this->nodes_[i]->id < 0)
      file << " [shape=circle];\n";
    else
      file << " [shape=hexagon];\n";
  }

  for (unsigned int i = 0; i < this->links_.size(); i++) {
    file << this->links_[i]->downNode->id << " -- " << this->links_[i]->upNode->id << ";\n";
  }
  file << "}";
  file.close();
}

FatTreeNode::FatTreeNode(ClusterCreationArgs* cluster, int id, int level, int position)
    : id(id), level(level), position(position)
{
  LinkCreationArgs linkTemplate;
  if (cluster->limiter_link) {
    linkTemplate.bandwidth = cluster->limiter_link;
    linkTemplate.latency   = 0;
    linkTemplate.policy    = SURF_LINK_SHARED;
    linkTemplate.id        = "limiter_"+std::to_string(id);
    sg_platf_new_link(&linkTemplate);
    this->limiterLink = surf::LinkImpl::byName(linkTemplate.id);
  }
  if (cluster->loopback_bw || cluster->loopback_lat) {
    linkTemplate.bandwidth = cluster->loopback_bw;
    linkTemplate.latency   = cluster->loopback_lat;
    linkTemplate.policy    = SURF_LINK_FATPIPE;
    linkTemplate.id        = "loopback_"+ std::to_string(id);
    sg_platf_new_link(&linkTemplate);
    this->loopback = surf::LinkImpl::byName(linkTemplate.id);
  }
}

FatTreeLink::FatTreeLink(ClusterCreationArgs* cluster, FatTreeNode* downNode, FatTreeNode* upNode)
    : upNode(upNode), downNode(downNode)
{
  static int uniqueId = 0;
  LinkCreationArgs linkTemplate;
  linkTemplate.bandwidth = cluster->bw;
  linkTemplate.latency   = cluster->lat;
  linkTemplate.policy    = cluster->sharing_policy; // sthg to do with that ?
  linkTemplate.id =
      "link_from_" + std::to_string(downNode->id) + "_" + std::to_string(upNode->id) + "_" + std::to_string(uniqueId);
  sg_platf_new_link(&linkTemplate);

  if (cluster->sharing_policy == SURF_LINK_FULLDUPLEX) {
    std::string tmpID = std::string(linkTemplate.id) + "_UP";
    this->upLink      = surf::LinkImpl::byName(tmpID); // check link?
    tmpID          = std::string(linkTemplate.id) + "_DOWN";
    this->downLink    = surf::LinkImpl::byName(tmpID); // check link ?
  } else {
    this->upLink   = surf::LinkImpl::byName(linkTemplate.id);
    this->downLink = this->upLink;
  }
  uniqueId++;
}
}
}
} // namespace
