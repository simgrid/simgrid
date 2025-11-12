/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/kernel/resource/NetworkModel.hpp"
#include "xbt/parse_units.hpp"

#include <numeric>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_routing_dragonfly, ker_platform, "Kernel Dragonfly Routing");

namespace simgrid {
namespace kernel::routing {

DragonflyZone::DragonflyZone(const std::string& name) : ClusterBase(name) {}

DragonflyZone::Coords DragonflyZone::rankId_to_coords(unsigned long rankId) const
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

void DragonflyZone::set_link_characteristics(double bw, double lat, s4u::Link::SharingPolicy sharing_policy)
{
  ClusterBase::set_link_characteristics(bw, lat, sharing_policy);
  if (sharing_policy == s4u::Link::SharingPolicy::SPLITDUPLEX)
    num_links_per_link_ = 2;
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

  std::vector<unsigned long> dimensions = {n_groups, n_chassis, n_routers, nodes};
  int tot_elements                     = std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<>());
  set_tot_elements(tot_elements); 
  set_cluster_dimensions(dimensions);
}

/* Generate the cluster once every node is created */
void DragonflyZone::build_upper_levels()
{
  generate_routers();
  generate_links();
}

void DragonflyZone::generate_routers()
{
  unsigned long id = 2UL * num_groups_ * num_chassis_per_group_ * num_blades_per_chassis_ * num_nodes_per_blade_;
  /* get limiter for this router */
  auto get_limiter = [this, &id](unsigned int i, unsigned int j,
                                                 unsigned int k) -> resource::StandardLinkImpl* {
    kernel::resource::StandardLinkImpl* limiter = nullptr;
    if (has_limiter()) {
      id--;
      const auto* s4u_link =
          get_limiter_cb()(get_iface(), {i, j, k, std::numeric_limits<unsigned int>::max()}, id);
      if (s4u_link) {
        limiter = s4u_link->get_impl();
      }
    }
    return limiter;
  };

  routers_.reserve(static_cast<size_t>(num_groups_) * num_chassis_per_group_ * num_blades_per_chassis_);
  for (unsigned int i = 0; i < num_groups_; i++) {
    for (unsigned int j = 0; j < num_chassis_per_group_; j++) {
      for (unsigned int k = 0; k < num_blades_per_chassis_; k++) {
        routers_.emplace_back(i, j, k, get_limiter(i, j, k));
      }
    }
  }
}

void DragonflyZone::generate_link(const std::string& id, int numlinks, resource::StandardLinkImpl** linkup,
                                  resource::StandardLinkImpl** linkdown)
{
  XBT_DEBUG("Generating link %s", id.c_str());
  *linkup   = nullptr;
  *linkdown = nullptr;
  if (get_link_sharing_policy() == s4u::Link::SharingPolicy::SPLITDUPLEX) {
    *linkup =
        add_link(id + "_UP", {get_link_bandwidth() * numlinks})->set_latency(get_link_latency())->seal()->get_impl();
    *linkdown = add_link(id + "_DOWN", {get_link_bandwidth() * numlinks})
                    ->set_latency(get_link_latency())
                    ->seal()
                    ->get_impl();
  } else {
    *linkup   = add_link(id, {get_link_bandwidth() * numlinks})->set_latency(get_link_latency())->seal()->get_impl();
    *linkdown = *linkup;
  }
}

void DragonflyZone::generate_links()
{
  static int uniqueId = 0;
  resource::StandardLinkImpl* linkup;
  resource::StandardLinkImpl* linkdown;

  unsigned int numRouters = num_groups_ * num_chassis_per_group_ * num_blades_per_chassis_;

  // Links from routers to their local nodes.
  for (unsigned int i = 0; i < numRouters; i++) {
    // allocate structures
    routers_[i].my_nodes_.resize(static_cast<size_t>(num_links_per_link_) * num_nodes_per_blade_);
    routers_[i].green_links_.resize(num_blades_per_chassis_);
    routers_[i].black_links_.resize(num_chassis_per_group_);

    for (unsigned int j = 0; j < num_links_per_link_ * num_nodes_per_blade_; j += num_links_per_link_) {
      std::string id = "local_link_from_router_" + std::to_string(i) + "_to_node_" +
                       std::to_string(j / num_links_per_link_) + "_" + std::to_string(uniqueId);
      generate_link(id, 1, &linkup, &linkdown);

      routers_[i].my_nodes_[j] = linkup;
      if (get_link_sharing_policy() == s4u::Link::SharingPolicy::SPLITDUPLEX)
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

void DragonflyZone::get_local_route(const NetPoint* src, const NetPoint* dst, Route* route, double* latency)
{
  // Minimal routing version.
  // TODO : non-minimal random one, and adaptive ?

  if (dst->is_router() || src->is_router())
    return;

  XBT_VERB("dragonfly getLocalRoute from '%s'[%lu] to '%s'[%lu]", src->get_cname(), src->id(), dst->get_cname(),
           dst->id());

  if ((src->id() == dst->id()) && has_loopback()) {
    resource::StandardLinkImpl* uplink = get_uplink_from(node_pos(src->id()));

    add_link_latency(route->link_list_, uplink, latency);
    return;
  }

  const auto myCoords     = rankId_to_coords(src->id());
  const auto targetCoords = rankId_to_coords(dst->id());
  XBT_DEBUG("src : %lu group, %lu chassis, %lu blade, %lu node", myCoords.group, myCoords.chassis, myCoords.blade,
            myCoords.node);
  XBT_DEBUG("dst : %lu group, %lu chassis, %lu blade, %lu node", targetCoords.group, targetCoords.chassis,
            targetCoords.blade, targetCoords.node);

  DragonflyRouter* myRouter      = &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                        myCoords.chassis * num_blades_per_chassis_ + myCoords.blade];
  DragonflyRouter* targetRouter  = &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                            targetCoords.chassis * num_blades_per_chassis_ + targetCoords.blade];
  DragonflyRouter* currentRouter = myRouter;

  if (has_limiter()) { // limiter for sender
    route->link_list_.push_back(get_uplink_from(node_pos_with_loopback(src->id())));
  }

  // node->router local link
  add_link_latency(route->link_list_, myRouter->my_nodes_[myCoords.node * num_links_per_link_], latency);

  if (targetRouter != myRouter) {
    // are we on a different group ?
    if (targetRouter->group_ != currentRouter->group_) {
      // go to the router of our group connected to this one.
      if (currentRouter->blade_ != targetCoords.group) {
        if (currentRouter->limiter_)
          route->link_list_.push_back(currentRouter->limiter_);
        // go to the nth router in our chassis
        add_link_latency(route->link_list_, currentRouter->green_links_[targetCoords.group], latency);
        currentRouter = &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) +
                                  myCoords.chassis * num_blades_per_chassis_ + targetCoords.group];
      }

      if (currentRouter->chassis_ != 0) {
        // go to the first chassis of our group
        if (currentRouter->limiter_)
          route->link_list_.push_back(currentRouter->limiter_);
        add_link_latency(route->link_list_, currentRouter->black_links_[0], latency);
        currentRouter =
            &routers_[myCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords.group];
      }

      // go to destination group - the only optical hop
      add_link_latency(route->link_list_, currentRouter->blue_link_, latency);
      if (currentRouter->limiter_)
        route->link_list_.push_back(currentRouter->limiter_);
      currentRouter =
          &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + myCoords.group];
    }

    // same group, but same blade ?
    if (targetRouter->blade_ != currentRouter->blade_) {
      if (currentRouter->limiter_)
        route->link_list_.push_back(currentRouter->limiter_);
      add_link_latency(route->link_list_, currentRouter->green_links_[targetCoords.blade], latency);
      currentRouter =
          &routers_[targetCoords.group * (num_chassis_per_group_ * num_blades_per_chassis_) + targetCoords.blade];
    }

    // same blade, but same chassis ?
    if (targetRouter->chassis_ != currentRouter->chassis_) {
      if (currentRouter->limiter_)
        route->link_list_.push_back(currentRouter->limiter_);
      add_link_latency(route->link_list_, currentRouter->black_links_[targetCoords.chassis], latency);
    }
  }

  // router->node local link
  if (targetRouter->limiter_)
    route->link_list_.push_back(targetRouter->limiter_);
  add_link_latency(route->link_list_,
                   targetRouter->my_nodes_[targetCoords.node * num_links_per_link_ + num_links_per_link_ - 1], latency);

  if (has_limiter()) { // limiter for receiver
    route->link_list_.push_back(get_downlink_to(node_pos_with_loopback(dst->id())));
  }

  // set gateways (if any)
  route->gw_src_ = get_gateway(src->id());
  route->gw_dst_ = get_gateway(dst->id());
}

void DragonflyZone::do_seal()
{
  /* populating it */
  for (int i = 0; i < get_tot_elements(); i++)
    fill_leaf_from_cb(i);

  build_upper_levels();
}

} // namespace kernel::routing

namespace s4u {

NetZone* create_dragonfly_zone(const std::string& name, NetZone* parent, const DragonflyParams& params,
  const ClusterCallbacks& set_callbacks, double bandwidth, double latency,
  Link::SharingPolicy sharing_policy)
{
  auto* zone = parent->add_netzone_dragonfly(name, params.groups, params.chassis, params.routers, params.nodes, bandwidth,
                                             latency, sharing_policy);
  if (set_callbacks.is_by_netzone())
    zone->set_netzone_cb(set_callbacks.netzone);
  else
    zone->set_host_cb(set_callbacks.host);
  zone->set_loopback_cb(set_callbacks.loopback);
  zone->set_limiter_cb(set_callbacks.limiter);

  return zone; 
}

NetZone* NetZone::add_netzone_dragonfly(const std::string& name, const std::pair<unsigned int, unsigned int>& groups,
                                        const std::pair<unsigned int, unsigned int>& chassis,
                                        const std::pair<unsigned int, unsigned int>& routers,
                                        unsigned int nodes, const std::string& bandwidth, const std::string& latency,
                                        Link::SharingPolicy sharing_policy)
{
  return add_netzone_dragonfly(name, groups, chassis, routers, nodes, xbt_parse_get_bandwidth("", 0, bandwidth, ""), 
                               xbt_parse_get_time("", 0, latency, ""), sharing_policy);
}
NetZone* NetZone::add_netzone_dragonfly(const std::string& name, const std::pair<unsigned int, unsigned int>& groups,
                                        const std::pair<unsigned int, unsigned int>& chassis,
                                        const std::pair<unsigned int, unsigned int>& routers,
                                        unsigned int nodes, double bandwidth, double latency,
                                        Link::SharingPolicy sharing_policy)
{
/* initial checks */
  if (bandwidth <= 0)
    throw std::invalid_argument("DragonflyZone: incorrect bandwidth for internode communication, bw=" +
                                std::to_string(bandwidth));
  if (latency < 0)
    throw std::invalid_argument("DragonflyZone: incorrect latency for internode communication, lat=" +
                                std::to_string(latency));

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
  
    /* creating zone */
  auto* zone = new kernel::routing::DragonflyZone(name);
  zone->set_topology(groups.first, groups.second, chassis.first, chassis.second, routers.first, routers.second, nodes);
  zone->set_parent(get_impl());
  zone->set_link_characteristics(bandwidth, latency, sharing_policy);

  return zone->get_iface();
}
} // namespace s4u

} // namespace simgrid
