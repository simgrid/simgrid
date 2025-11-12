/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/FatTreeZone.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>

#include "src/kernel/resource/NetworkModel.hpp"
#include "src/kernel/xml/platf.hpp" // simgrid_parse_error() and simgrid_parse_assert()
#include "xbt/asserts.hpp"
#include "xbt/parse_units.hpp"

#include <fstream>
#include <numeric>
#include <sstream>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_routing_fat_tree, ker_platform, "Kernel Fat-Tree Routing");

namespace simgrid {
namespace kernel::routing {

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

    int k = this->num_parents_per_node_[currentNode->level] * this->num_port_lower_level_[currentNode->level];
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
    // pick cable when multiple parallels
    int d = source->position % this->num_port_lower_level_[currentNode->level - 1];
    for (unsigned int i = d * this->num_children_per_node_[currentNode->level - 1]; i < currentNode->children.size();
         i++) {
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

void FatTreeZone::build_upper_levels()
{
  generate_switches();
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
  /* populating the Fat-Tree */
  for (int i = 0; i < get_tot_elements(); i++) {
    /* coordinates are based on 2 indexes: number of levels and id */
    auto [netpoint, loopback, limiter] = fill_leaf_from_cb(i);
    add_processing_node(i, limiter ? limiter->get_impl() : nullptr, loopback ? loopback->get_impl() : nullptr);
  }
  build_upper_levels();

  if (this->levels_ == 0) {
    return;
  }
  if (not XBT_LOG_ISENABLED(ker_routing_fat_tree, xbt_log_priority_debug)) {
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
        this->add_internal_link(currentParentNode->get(), node->label[level] + j * this->num_children_per_node_[level],
                                node, (*currentParentNode)->label[level] + j * this->num_parents_per_node_[level]);
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

  if (XBT_LOG_ISENABLED(ker_routing_fat_tree, xbt_log_priority_debug)) {
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
    simgrid_parse_error("The number of provided nodes does not fit with the wanted topology."
                        " Please check your platform description (We need " +
                        std::to_string(this->nodes_by_level_[0]) + "nodes, we got " +
                        std::to_string(this->nodes_.size()));
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
  auto get_limiter = [this](unsigned long i, unsigned long j, long id) -> resource::StandardLinkImpl* {
    kernel::resource::StandardLinkImpl* limiter = nullptr;
    if (has_limiter()) {
      const auto* s4u_link = get_limiter_cb()(get_iface(), {i + 1, j}, id);
      if (s4u_link) {
        limiter = s4u_link->get_impl();
      }
    }
    return limiter;
  };
  // Create the switches
  unsigned long k = 2 * nodes_.size();
  for (unsigned long i = 0; i < this->levels_; i++) {
    for (unsigned long j = 0; j < this->nodes_by_level_[i + 1]; j++) {
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
      if (XBT_LOG_ISENABLED(ker_routing_fat_tree, xbt_log_priority_debug)) {
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

void FatTreeZone::add_processing_node(int id, resource::StandardLinkImpl* limiter, resource::StandardLinkImpl* loopback)
{
  using std::make_pair;
  static int position = 0;
  auto newNode        = std::make_shared<FatTreeNode>(id, 0, position, limiter, loopback);
  position++;
  newNode->parents.resize(static_cast<size_t>(this->num_parents_per_node_[0]) * this->num_port_lower_level_[0]);
  newNode->label.resize(this->levels_);
  this->compute_nodes_.try_emplace(id, newNode);
  this->nodes_.emplace_back(newNode);
}

void FatTreeZone::add_internal_link(FatTreeNode* parent, unsigned int parentPort, FatTreeNode* child,
                                    unsigned int childPort)
{
  static int uniqueId = 0;
  const s4u::Link* linkup;
  const s4u::Link* linkdown;
  std::string id =
      "link_from_" + std::to_string(child->id) + "_" + std::to_string(parent->id) + "_" + std::to_string(uniqueId);

  if (get_link_sharing_policy() == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    linkup   = add_link(id + "_UP", {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
    linkdown = add_link(id + "_DOWN", {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
  } else {
    linkup   = add_link(id, {get_link_bandwidth()})->set_latency(get_link_latency())->seal();
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
  unsigned int tot_elements = std::accumulate(down_links.begin(), down_links.end(), 1, std::multiplies<>());
  set_tot_elements(tot_elements); 
  set_cluster_dimensions({n_levels + 1, tot_elements});
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
} // namespace kernel::routing

namespace s4u {

NetZone* create_fatTree_zone(const std::string& name, NetZone* parent, const FatTreeParams& params,
                             const ClusterCallbacks& set_callbacks, double bandwidth, double latency,
                             Link::SharingPolicy sharing_policy) // XBT_ATTRIB_DEPRECATED_v401
{
  auto zone = parent->add_netzone_fatTree(name, params.levels, params.down, params.up, params.count, bandwidth, 
                                          latency, sharing_policy);
  if (set_callbacks.is_by_netzone())
    zone->set_netzone_cb(set_callbacks.netzone);
  else
    zone->set_host_cb(set_callbacks.host);
  zone->set_loopback_cb(set_callbacks.loopback);
  zone->set_limiter_cb(set_callbacks.limiter);
  return zone;
}

NetZone* NetZone::add_netzone_fatTree(const std::string& name, unsigned int n_levels,
                                      const std::vector<unsigned int>& down_links,
                                      const std::vector<unsigned int>& up_links,
                                      const std::vector<unsigned int>& link_counts, const std::string& bandwidth,
                                      const std::string& latency, Link::SharingPolicy sharing_policy)
{
  return add_netzone_fatTree(name, n_levels, down_links, up_links, link_counts,
                             xbt_parse_get_bandwidth("", 0, bandwidth, ""), xbt_parse_get_time("", 0, latency, ""),
                             sharing_policy);
}

NetZone* NetZone::add_netzone_fatTree(const std::string& name, unsigned int n_levels,
                                      const std::vector<unsigned int>& down_links,
                                      const std::vector<unsigned int>& up_links,
                                      const std::vector<unsigned int>& link_counts, double bandwidth, double latency,
                                      Link::SharingPolicy sharing_policy)
{
  /* initial checks */
  if (bandwidth <= 0)
    throw std::invalid_argument("FatTreeZone: incorrect bandwidth for internode communication, bw=" +
                                std::to_string(bandwidth));
  if (latency < 0)
    throw std::invalid_argument("FatTreeZone: incorrect latency for internode communication, lat=" +
                                std::to_string(latency));

  kernel::routing::FatTreeZone::check_topology(n_levels, down_links, up_links, link_counts);

  /* creating zone */
  auto* zone = new kernel::routing::FatTreeZone(name);
  zone->set_topology(n_levels, down_links, up_links, link_counts);
  zone->set_parent(this->get_impl());
  zone->set_link_characteristics(bandwidth, latency, sharing_policy);

  return zone->get_iface();
}
} // namespace s4u

} // namespace simgrid
