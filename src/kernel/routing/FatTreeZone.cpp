/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <fstream>
#include <numeric>
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

bool FatTreeZone::is_in_sub_tree(const FatTreeNode* root, const FatTreeNode* node) const
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

void FatTreeZone::get_local_route(const NetPoint* src, const NetPoint* dst, Route* into, double* latency)
{
  if (dst->is_router() || src->is_router())
    return;

  /* Let's find the source and the destination in our internal structure */
  auto searchedNode = this->compute_nodes_.find(src->id());
  xbt_assert(searchedNode != this->compute_nodes_.end(), "Could not find the source %s [%lu] in the fat tree",
             src->get_cname(), src->id());
  const FatTreeNode* source = searchedNode->second.get();

  searchedNode = this->compute_nodes_.find(dst->id());
  xbt_assert(searchedNode != this->compute_nodes_.end(), "Could not find the destination %s [%lu] in the fat tree",
             dst->get_cname(), dst->id());
  const FatTreeNode* destination = searchedNode->second.get();

  XBT_VERB("Get route and latency from '%s' [%lu] to '%s' [%lu] in a fat tree", src->get_cname(), src->id(),
           dst->get_cname(), dst->id());

  /* In case destination is the source, and there is a loopback, let's use it instead of going up to a switch */
  if (source->id == destination->id && has_loopback()) {
    add_link_latency(into->link_list_, source->loopback_, latency);
    return;
  }

  const FatTreeNode* currentNode = source;

  // up part
  while (not is_in_sub_tree(currentNode, destination)) {
    int d = destination->position; // as in d-mod-k

    for (unsigned int i = 0; i < currentNode->level; i++)
      d /= this->num_parents_per_node_[i];

    int k = this->num_parents_per_node_[currentNode->level];
    d     = d % k;

    if (currentNode->limiter_link_)
      into->link_list_.push_back(currentNode->limiter_link_);

    add_link_latency(into->link_list_, currentNode->parents[d]->up_link_, latency);

    currentNode = currentNode->parents[d]->up_node_;
  }

  XBT_DEBUG("%d(%u,%u) is in the sub tree of %d(%u,%u).", destination->id, destination->level, destination->position,
            currentNode->id, currentNode->level, currentNode->position);

  // Down part
  while (currentNode != destination) {
    for (unsigned int i = 0; i < currentNode->children.size(); i++) {
      if (i % this->num_children_per_node_[currentNode->level - 1] == destination->label[currentNode->level - 1]) {
        add_link_latency(into->link_list_, currentNode->children[i]->down_link_, latency);

        if (currentNode->limiter_link_)
          into->link_list_.push_back(currentNode->limiter_link_);

        currentNode = currentNode->children[i]->down_node_;
        XBT_DEBUG("%d(%u,%u) is accessible through %d(%u,%u)", destination->id, destination->level,
                  destination->position, currentNode->id, currentNode->level, currentNode->position);
      }
    }
  }
  if (currentNode->limiter_link_) { // limiter for receiver/destination
    into->link_list_.push_back(currentNode->limiter_link_);
  }
  // set gateways (if any)
  into->gw_src_ = get_gateway(src->id());
  into->gw_dst_ = get_gateway(dst->id());
}

void FatTreeZone::build_upper_levels(const s4u::ClusterCallbacks& set_callbacks)
{
  generate_switches(set_callbacks);
  generate_labels();

  unsigned int k = 0;
  // Nodes are totally ordered, by level and then by position, in this->nodes
  for (unsigned int i = 0; i < levels_; i++) {
    for (unsigned int j = 0; j < nodes_by_level_[i]; j++) {
      connect_node_to_parents(nodes_[k].get());
      k++;
    }
  }
}

void FatTreeZone::do_seal()
{
  if (this->levels_ == 0) {
    return;
  }
  if (not XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    return;
  }

  /* for debugging purpose only, Fat-Tree is already build when seal is called */
  std::stringstream msgBuffer;

  msgBuffer << "We are creating a fat tree of " << this->levels_ << " levels "
            << "with " << this->nodes_by_level_[0] << " processing nodes";
  for (unsigned int i = 1; i <= this->levels_; i++) {
    msgBuffer << ", " << this->nodes_by_level_[i] << " switches at level " << i;
  }
  XBT_DEBUG("%s", msgBuffer.str().c_str());
  msgBuffer.str("");
  msgBuffer << "Nodes are : ";

  for (auto const& node : this->nodes_) {
    msgBuffer << node->id << "(" << node->level << "," << node->position << ") ";
  }
  XBT_DEBUG("%s", msgBuffer.str().c_str());

  msgBuffer.clear();
  msgBuffer << "Links are : ";
  for (auto const& link : this->links_) {
    msgBuffer << "(" << link->up_node_->id << "," << link->down_node_->id << ") ";
  }
  XBT_DEBUG("%s", msgBuffer.str().c_str());
}

int FatTreeZone::connect_node_to_parents(FatTreeNode* node)
{
  auto currentParentNode = this->nodes_.begin();
  int connectionsNumber  = 0;
  const int level        = node->level;
  XBT_DEBUG("We are connecting node %d(%u,%u) to his parents.", node->id, node->level, node->position);
  currentParentNode += this->get_level_position(level + 1);
  for (unsigned int i = 0; i < this->nodes_by_level_[level + 1]; i++) {
    if (this->are_related(currentParentNode->get(), node)) {
      XBT_DEBUG("%d(%u,%u) and %d(%u,%u) are related,"
                " with %u links between them.",
                node->id, node->level, node->position, (*currentParentNode)->id, (*currentParentNode)->level,
                (*currentParentNode)->position, this->num_port_lower_level_[level]);
      for (unsigned int j = 0; j < this->num_port_lower_level_[level]; j++) {
        this->add_link(currentParentNode->get(), node->label[level] + j * this->num_children_per_node_[level], node,
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

void FatTreeZone::generate_switches(const s4u::ClusterCallbacks& set_callbacks)
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

  /* get limiter for this router */
  auto get_limiter = [this, &set_callbacks](unsigned int i, unsigned int j, int id) -> resource::LinkImpl* {
    kernel::resource::LinkImpl* limiter = nullptr;
    if (set_callbacks.limiter) {
      const auto* s4u_link = set_callbacks.limiter(get_iface(), {i + 1, j}, id);
      if (s4u_link) {
        limiter = s4u_link->get_impl();
      }
    }
    return limiter;
  };
  // Create the switches
  int k = 0;
  for (unsigned int i = 0; i < this->levels_; i++) {
    for (unsigned int j = 0; j < this->nodes_by_level_[i + 1]; j++) {
      k--;
      auto newNode = std::make_shared<FatTreeNode>(k, i + 1, j, get_limiter(i, j, k), nullptr);
      XBT_DEBUG("We create the switch %d(%u,%u)", newNode->id, newNode->level, newNode->position);
      newNode->children.resize(static_cast<size_t>(this->num_children_per_node_[i]) * this->num_port_lower_level_[i]);
      if (i != this->levels_ - 1) {
        newNode->parents.resize(static_cast<size_t>(this->num_parents_per_node_[i + 1]) *
                                this->num_port_lower_level_[i + 1]);
      }
      newNode->label.resize(this->levels_);
      this->nodes_.emplace_back(newNode);
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

void FatTreeZone::add_processing_node(int id, resource::LinkImpl* limiter, resource::LinkImpl* loopback)
{
  using std::make_pair;
  static int position = 0;
  auto newNode = std::make_shared<FatTreeNode>(id, 0, position, limiter, loopback);
  position++;
  newNode->parents.resize(static_cast<size_t>(this->num_parents_per_node_[0]) * this->num_port_lower_level_[0]);
  newNode->label.resize(this->levels_);
  this->compute_nodes_.insert(make_pair(id, newNode));
  this->nodes_.emplace_back(newNode);
}

void FatTreeZone::add_link(FatTreeNode* parent, unsigned int parentPort, FatTreeNode* child, unsigned int childPort)
{
  static int uniqueId = 0;
  const s4u::Link* linkup;
  const s4u::Link* linkdown;
  std::string id =
      "link_from_" + std::to_string(child->id) + "_" + std::to_string(parent->id) + "_" + std::to_string(uniqueId);

  if (get_link_sharing_policy() == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    linkup   = create_link(id + "_UP", {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
    linkdown = create_link(id + "_DOWN", {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
  } else {
    linkup   = create_link(id, {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
    linkdown = linkup;
  }
  uniqueId++;

  auto newLink = std::make_shared<FatTreeLink>(child, parent, linkup->get_impl(), linkdown->get_impl());
  XBT_DEBUG("Creating a link between the parent (%u,%u,%u) and the child (%u,%u,%u)", parent->level, parent->position,
            parentPort, child->level, child->position, childPort);
  parent->children[parentPort] = newLink;
  child->parents[childPort]    = newLink;

  this->links_.emplace_back(newLink);
}

void FatTreeZone::check_topology(unsigned int n_levels, const std::vector<unsigned int>& down_links,
                                 const std::vector<unsigned int>& up_links, const std::vector<unsigned int>& link_count)

{
  /* check number of levels */
  if (n_levels == 0)
    throw std::invalid_argument("FatTreeZone: invalid number of levels, must be > 0");

  auto check_vector = [&n_levels](const std::vector<unsigned int>& vector, const std::string& var_name) {
    if (vector.size() != n_levels)
      throw std::invalid_argument("FatTreeZone: invalid " + var_name + " parameter, vector has " +
                                  std::to_string(vector.size()) + " elements, must have " + std::to_string(n_levels));

    auto check_zero = [](unsigned int i) { return i == 0; };
    if (std::any_of(vector.begin(), vector.end(), check_zero))
      throw std::invalid_argument("FatTreeZone: invalid " + var_name + " parameter, all values must be greater than 0");
  };

  /* check remaining vectors */
  check_vector(down_links, "down links");
  check_vector(up_links, "up links");
  check_vector(link_count, "link count");
}

void FatTreeZone::set_topology(unsigned int n_levels, const std::vector<unsigned int>& down_links,
                               const std::vector<unsigned int>& up_links, const std::vector<unsigned int>& link_count)
{
  levels_                = n_levels;
  num_children_per_node_ = down_links;
  num_parents_per_node_  = up_links;
  num_port_lower_level_  = link_count;
}

s4u::FatTreeParams FatTreeZone::parse_topo_parameters(const std::string& topo_parameters)
{
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  unsigned int n_lev = 0;
  std::vector<unsigned int> down;
  std::vector<unsigned int> up;
  std::vector<unsigned int> count;
  boost::split(parameters, topo_parameters, boost::is_any_of(";"));

  surf_parse_assert(
      parameters.size() == 4,
      "Fat trees are defined by the levels number and 3 vectors, see the documentation for more information.");

  // The first parts of topo_parameters should be the levels number
  try {
    n_lev = std::stoi(parameters[0]);
  } catch (const std::invalid_argument&) {
    surf_parse_error(std::string("First parameter is not the amount of levels: ") + parameters[0]);
  }

  // Then, a l-sized vector standing for the children number by level
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == n_lev, std::string("You specified ") + std::to_string(n_lev) +
                                             " levels but the child count vector (the first one) contains " +
                                             std::to_string(tmp.size()) + " levels.");

  for (std::string const& level : tmp) {
    try {
      down.push_back(std::stoi(level));
    } catch (const std::invalid_argument&) {
      surf_parse_error(std::string("Invalid child count: ") + level);
    }
  }

  // Then, a l-sized vector standing for the parents number by level
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == n_lev, std::string("You specified ") + std::to_string(n_lev) +
                                             " levels but the parent count vector (the second one) contains " +
                                             std::to_string(tmp.size()) + " levels.");
  for (std::string const& parent : tmp) {
    try {
      up.push_back(std::stoi(parent));
    } catch (const std::invalid_argument&) {
      surf_parse_error(std::string("Invalid parent count: ") + parent);
    }
  }

  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  surf_parse_assert(tmp.size() == n_lev, std::string("You specified ") + std::to_string(n_lev) +
                                             " levels but the port count vector (the third one) contains " +
                                             std::to_string(tmp.size()) + " levels.");
  for (std::string const& port : tmp) {
    try {
      count.push_back(std::stoi(port));
    } catch (const std::invalid_argument&) {
      throw std::invalid_argument(std::string("Invalid lower level port number:") + port);
    }
  }
  return s4u::FatTreeParams(n_lev, down, up, count);
}

void FatTreeZone::generate_dot_file(const std::string& filename) const
{
  std::ofstream file;
  file.open(filename, std::ios::out | std::ios::trunc);
  xbt_assert(file.is_open(), "Unable to open file %s", filename.c_str());

  file << "graph AsClusterFatTree {\n";
  for (auto const& node : this->nodes_) {
    file << node->id;
    if (node->id < 0)
      file << " [shape=circle];\n";
    else
      file << " [shape=hexagon];\n";
  }

  for (auto const& link : this->links_) {
    file << link->down_node_->id << " -- " << link->up_node_->id << ";\n";
  }
  file << "}";
  file.close();
}
} // namespace routing
} // namespace kernel

namespace s4u {
FatTreeParams::FatTreeParams(unsigned int n_levels, const std::vector<unsigned int>& down_links,
                             const std::vector<unsigned int>& up_links, const std::vector<unsigned int>& links_number)
    : levels(n_levels), down(down_links), up(up_links), number(links_number)
{
  kernel::routing::FatTreeZone::check_topology(levels, down, up, number);
}

NetZone* create_fatTree_zone(const std::string& name, const NetZone* parent, const FatTreeParams& params,
                             const ClusterCallbacks& set_callbacks, double bandwidth, double latency,
                             Link::SharingPolicy sharing_policy)
{
  /* initial checks */
  if (bandwidth <= 0)
    throw std::invalid_argument("FatTreeZone: incorrect bandwidth for internode communication, bw=" +
                                std::to_string(bandwidth));
  if (latency < 0)
    throw std::invalid_argument("FatTreeZone: incorrect latency for internode communication, lat=" +
                                std::to_string(latency));

  /* creating zone */
  auto* zone = new kernel::routing::FatTreeZone(name);
  zone->set_topology(params.levels, params.down, params.up, params.number);
  if (parent)
    zone->set_parent(parent->get_impl());
  zone->set_link_characteristics(bandwidth, latency, sharing_policy);

  /* populating it */
  unsigned int tot_elements = std::accumulate(params.down.begin(), params.down.end(), 1, std::multiplies<>());
  for (unsigned int i = 0; i < tot_elements; i++) {
    kernel::routing::NetPoint* netpoint;
    Link* limiter;
    Link* loopback;
    /* coordinates are based on 2 indexes: number of levels and id */
    zone->fill_leaf_from_cb(i, {params.levels + 1, tot_elements}, set_callbacks, &netpoint, &loopback, &limiter);
    zone->add_processing_node(i, limiter ? limiter->get_impl() : nullptr, loopback ? loopback->get_impl() : nullptr);
  }
  zone->build_upper_levels(set_callbacks);

  return zone->get_iface();
}
} // namespace s4u

} // namespace simgrid
