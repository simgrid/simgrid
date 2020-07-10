/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <fstream>
#include <sstream>
#include <string>

#include "simgrid/kernel/routing/FatTreeZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"


#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_fat_tree, surf, "Routing for fat trees");

namespace simgrid {
namespace kernel {
namespace routing {

FatTreeZone::FatTreeZone(NetZoneImpl* father, const std::string& name, resource::NetworkModel* netmodel)
    : ClusterZone(father, name, netmodel)
{
  XBT_DEBUG("Creating a new fat tree.");
}

FatTreeZone::~FatTreeZone()
{
  for (FatTreeNode const* node : this->nodes_) {
    delete node;
  }
  for (FatTreeLink const* link : this->links_) {
    delete link;
  }
}

bool FatTreeZone::is_in_sub_tree(FatTreeNode* root, FatTreeNode* node) const
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

void FatTreeZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* into, double* latency)
{
  if (dst->is_router() || src->is_router())
    return;

  /* Let's find the source and the destination in our internal structure */
  auto searchedNode = this->compute_nodes_.find(src->id());
  xbt_assert(searchedNode != this->compute_nodes_.end(), "Could not find the source %s [%u] in the fat tree",
             src->get_cname(), src->id());
  FatTreeNode* source = searchedNode->second;

  searchedNode = this->compute_nodes_.find(dst->id());
  xbt_assert(searchedNode != this->compute_nodes_.end(), "Could not find the destination %s [%u] in the fat tree",
             dst->get_cname(), dst->id());
  FatTreeNode* destination = searchedNode->second;

  XBT_VERB("Get route and latency from '%s' [%u] to '%s' [%u] in a fat tree", src->get_cname(), src->id(),
           dst->get_cname(), dst->id());

  /* In case destination is the source, and there is a loopback, let's use it instead of going up to a switch */
  if (source->id == destination->id && this->has_loopback_) {
    into->link_list.push_back(source->loopback);
    if (latency)
      *latency += source->loopback->get_latency();
    return;
  }

  FatTreeNode* currentNode = source;

  // up part
  while (not is_in_sub_tree(currentNode, destination)) {
    int d = destination->position; // as in d-mod-k

    for (unsigned int i = 0; i < currentNode->level; i++)
      d /= this->num_parents_per_node_[i];

    int k = this->num_parents_per_node_[currentNode->level];
    d     = d % k;
    into->link_list.push_back(currentNode->parents[d]->up_link_);

    if (latency)
      *latency += currentNode->parents[d]->up_link_->get_latency();

    if (this->has_limiter_)
      into->link_list.push_back(currentNode->limiter_link_);
    currentNode = currentNode->parents[d]->up_node_;
  }

  XBT_DEBUG("%d(%u,%u) is in the sub tree of %d(%u,%u).", destination->id, destination->level, destination->position,
            currentNode->id, currentNode->level, currentNode->position);

  // Down part
  while (currentNode != destination) {
    for (unsigned int i = 0; i < currentNode->children.size(); i++) {
      if (i % this->num_children_per_node_[currentNode->level - 1] == destination->label[currentNode->level - 1]) {
        into->link_list.push_back(currentNode->children[i]->down_link_);
        if (latency)
          *latency += currentNode->children[i]->down_link_->get_latency();
        currentNode = currentNode->children[i]->down_node_;
        if (this->has_limiter_)
          into->link_list.push_back(currentNode->limiter_link_);
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
  this->generate_switches();

  if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;

    msgBuffer << "We are creating a fat tree of " << this->levels_ << " levels "
              << "with " << this->nodes_by_level_[0] << " processing nodes";
    for (unsigned int i = 1; i <= this->levels_; i++) {
      msgBuffer << ", " << this->nodes_by_level_[i] << " switches at level " << i;
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
    msgBuffer.str("");
    msgBuffer << "Nodes are : ";

    for (FatTreeNode const* node : this->nodes_) {
      msgBuffer << node->id << "(" << node->level << "," << node->position << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }

  this->generate_labels();

  unsigned int k = 0;
  // Nodes are totally ordered, by level and then by position, in this->nodes
  for (unsigned int i = 0; i < this->levels_; i++) {
    for (unsigned int j = 0; j < this->nodes_by_level_[i]; j++) {
      this->connect_node_to_parents(this->nodes_[k]);
      k++;
    }
  }

  if (XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;
    msgBuffer << "Links are : ";
    for (FatTreeLink const* link : this->links_) {
      msgBuffer << "(" << link->up_node_->id << "," << link->down_node_->id << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }
}

int FatTreeZone::connect_node_to_parents(FatTreeNode* node)
{
  std::vector<FatTreeNode*>::iterator currentParentNode = this->nodes_.begin();
  int connectionsNumber                                 = 0;
  const int level                                       = node->level;
  XBT_DEBUG("We are connecting node %d(%u,%u) to his parents.", node->id, node->level, node->position);
  currentParentNode += this->get_level_position(level + 1);
  for (unsigned int i = 0; i < this->nodes_by_level_[level + 1]; i++) {
    if (this->are_related(*currentParentNode, node)) {
      XBT_DEBUG("%d(%u,%u) and %d(%u,%u) are related,"
                " with %u links between them.",
                node->id, node->level, node->position, (*currentParentNode)->id, (*currentParentNode)->level,
                (*currentParentNode)->position, this->num_port_lower_level_[level]);
      for (unsigned int j = 0; j < this->num_port_lower_level_[level]; j++) {
        this->add_link(*currentParentNode, node->label[level] + j * this->num_children_per_node_[level], node,
                       (*currentParentNode)->label[level] + j * this->num_parents_per_node_[level]);
      }
      connectionsNumber++;
    }
    ++currentParentNode;
  }
  return connectionsNumber;
}

bool FatTreeZone::are_related(FatTreeNode* parent, FatTreeNode* child) const
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

void FatTreeZone::generate_switches()
{
  XBT_DEBUG("Generating switches.");
  this->nodes_by_level_.resize(this->levels_ + 1, 0);

  // Take care of the number of nodes by level
  this->nodes_by_level_[0] = 1;
  for (unsigned int i = 0; i < this->levels_; i++)
    this->nodes_by_level_[0] *= this->num_children_per_node_[i];

  if (this->nodes_by_level_[0] != this->nodes_.size()) {
    surf_parse_error(std::string("The number of provided nodes does not fit with the wanted topology.") +
                     " Please check your platform description (We need " + std::to_string(this->nodes_by_level_[0]) +
                     "nodes, we got " + std::to_string(this->nodes_.size()));
  }

  for (unsigned int i = 0; i < this->levels_; i++) {
    int nodesInThisLevel = 1;

    for (unsigned int j = 0; j <= i; j++)
      nodesInThisLevel *= this->num_parents_per_node_[j];

    for (unsigned int j = i + 1; j < this->levels_; j++)
      nodesInThisLevel *= this->num_children_per_node_[j];

    this->nodes_by_level_[i + 1] = nodesInThisLevel;
  }

  // Create the switches
  int k = 0;
  for (unsigned int i = 0; i < this->levels_; i++) {
    for (unsigned int j = 0; j < this->nodes_by_level_[i + 1]; j++) {
      FatTreeNode* newNode = new FatTreeNode(this->cluster_, --k, i + 1, j);
      XBT_DEBUG("We create the switch %d(%u,%u)", newNode->id, newNode->level, newNode->position);
      newNode->children.resize(this->num_children_per_node_[i] * this->num_port_lower_level_[i]);
      if (i != this->levels_ - 1) {
        newNode->parents.resize(this->num_parents_per_node_[i + 1] * this->num_port_lower_level_[i + 1]);
      }
      newNode->label.resize(this->levels_);
      this->nodes_.push_back(newNode);
    }
  }
}

void FatTreeZone::generate_labels()
{
  XBT_DEBUG("Generating labels.");
  // TODO : check if nodesByLevel and nodes are filled
  std::vector<int> maxLabel(this->levels_);
  std::vector<int> currentLabel(this->levels_);
  unsigned int k = 0;
  for (unsigned int i = 0; i <= this->levels_; i++) {
    currentLabel.assign(this->levels_, 0);
    for (unsigned int j = 0; j < this->levels_; j++) {
      maxLabel[j] = j + 1 > i ? this->num_children_per_node_[j] : this->num_parents_per_node_[j];
    }

    for (unsigned int j = 0; j < this->nodes_by_level_[i]; j++) {
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

int FatTreeZone::get_level_position(const unsigned int level)
{
  xbt_assert(level <= this->levels_, "The impossible did happen. Yet again.");
  int tempPosition = 0;

  for (unsigned int i = 0; i < level; i++)
    tempPosition += this->nodes_by_level_[i];

  return tempPosition;
}

void FatTreeZone::add_processing_node(int id)
{
  using std::make_pair;
  static int position = 0;
  FatTreeNode* newNode;
  newNode = new FatTreeNode(this->cluster_, id, 0, position++);
  newNode->parents.resize(this->num_parents_per_node_[0] * this->num_port_lower_level_[0]);
  newNode->label.resize(this->levels_);
  this->compute_nodes_.insert(make_pair(id, newNode));
  this->nodes_.push_back(newNode);
}

void FatTreeZone::add_link(FatTreeNode* parent, unsigned int parentPort, FatTreeNode* child, unsigned int childPort)
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

  // TODO : we have to check for zeros and negative numbers, or it might crash
  surf_parse_assert(
      parameters.size() == 4,
      "Fat trees are defined by the levels number and 3 vectors, see the documentation for more information.");

  // The first parts of topo_parameters should be the levels number
  try {
    this->levels_ = std::stoi(parameters[0]);
  } catch (const std::invalid_argument&) {
    surf_parse_error(std::string("First parameter is not the amount of levels: ") + parameters[0]);
  }

  // Then, a l-sized vector standing for the children number by level
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == this->levels_, std::string("You specified ") + std::to_string(this->levels_) +
                                                     " levels but the child count vector (the first one) contains " +
                                                     std::to_string(tmp.size()) + " levels.");

  for (std::string const& level : tmp) {
    try {
      this->num_children_per_node_.push_back(std::stoi(level));
    } catch (const std::invalid_argument&) {
      surf_parse_error(std::string("Invalid child count: ") + level);
    }
  }

  // Then, a l-sized vector standing for the parents number by level
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == this->levels_, std::string("You specified ") + std::to_string(this->levels_) +
                                                     " levels but the parent count vector (the second one) contains " +
                                                     std::to_string(tmp.size()) + " levels.");
  for (std::string const& parent : tmp) {
    try {
      this->num_parents_per_node_.push_back(std::stoi(parent));
    } catch (const std::invalid_argument&) {
      surf_parse_error(std::string("Invalid parent count: ") + parent);
    }
  }

  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == this->levels_, std::string("You specified ") + std::to_string(this->levels_) +
                                                     " levels but the port count vector (the third one) contains " +
                                                     std::to_string(tmp.size()) + " levels.");
  for (std::string const& port : tmp) {
    try {
      this->num_port_lower_level_.push_back(std::stoi(port));
    } catch (const std::invalid_argument&) {
      throw std::invalid_argument(std::string("Invalid lower level port number:") + port);
    }
  }
  this->cluster_ = cluster;
}

void FatTreeZone::generate_dot_file(const std::string& filename) const
{
  std::ofstream file;
  file.open(filename, std::ios::out | std::ios::trunc);
  xbt_assert(file.is_open(), "Unable to open file %s", filename.c_str());

  file << "graph AsClusterFatTree {\n";
  for (FatTreeNode const* node : this->nodes_) {
    file << node->id;
    if (node->id < 0)
      file << " [shape=circle];\n";
    else
      file << " [shape=hexagon];\n";
  }

  for (FatTreeLink const* link : this->links_) {
    file << link->down_node_->id << " -- " << link->up_node_->id << ";\n";
  }
  file << "}";
  file.close();
}

FatTreeNode::FatTreeNode(const ClusterCreationArgs* cluster, int id, int level, int position)
    : id(id), level(level), position(position)
{
  LinkCreationArgs linkTemplate;
  if (cluster->limiter_link != 0.0) {
    linkTemplate.bandwidths.push_back(cluster->limiter_link);
    linkTemplate.latency   = 0;
    linkTemplate.policy    = s4u::Link::SharingPolicy::SHARED;
    linkTemplate.id        = "limiter_"+std::to_string(id);
    sg_platf_new_link(&linkTemplate);
    this->limiter_link_ = s4u::Link::by_name(linkTemplate.id)->get_impl();
  }
  if (cluster->loopback_bw != 0.0 || cluster->loopback_lat != 0.0) {
    linkTemplate.bandwidths.push_back(cluster->loopback_bw);
    linkTemplate.latency   = cluster->loopback_lat;
    linkTemplate.policy    = s4u::Link::SharingPolicy::FATPIPE;
    linkTemplate.id        = "loopback_"+ std::to_string(id);
    sg_platf_new_link(&linkTemplate);
    this->loopback = s4u::Link::by_name(linkTemplate.id)->get_impl();
  }
}

FatTreeLink::FatTreeLink(const ClusterCreationArgs* cluster, FatTreeNode* downNode, FatTreeNode* upNode)
    : up_node_(upNode), down_node_(downNode)
{
  static int uniqueId = 0;
  LinkCreationArgs linkTemplate;
  linkTemplate.bandwidths.push_back(cluster->bw);
  linkTemplate.latency   = cluster->lat;
  linkTemplate.policy    = cluster->sharing_policy; // sthg to do with that ?
  linkTemplate.id =
      "link_from_" + std::to_string(downNode->id) + "_" + std::to_string(upNode->id) + "_" + std::to_string(uniqueId);
  sg_platf_new_link(&linkTemplate);

  if (cluster->sharing_policy == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    this->up_link_   = s4u::Link::by_name(linkTemplate.id + "_UP")->get_impl();   // check link?
    this->down_link_ = s4u::Link::by_name(linkTemplate.id + "_DOWN")->get_impl(); // check link ?
  } else {
    this->up_link_   = s4u::Link::by_name(linkTemplate.id)->get_impl();
    this->down_link_ = this->up_link_;
  }
  uniqueId++;
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
