/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp" // FIXME: RouteCreationArgs and friends

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster, surf, "Routing part of surf");

/* This routing is specifically setup to represent clusters, aka homogeneous sets of machines
 * Note that a router is created, easing the interconnection with the rest of the world. */

namespace simgrid {
namespace kernel {
namespace routing {
ClusterZone::ClusterZone(const std::string& name) : NetZoneImpl(name) {}

void ClusterZone::set_loopback()
{
  if (not has_loopback_) {
    num_links_per_node_++;
    has_loopback_ = true;
  }
}

void ClusterZone::set_limiter()
{
  if (not has_limiter_) {
    num_links_per_node_++;
    has_limiter_ = true;
  }
}

void ClusterZone::add_private_link_at(unsigned int position, std::pair<resource::LinkImpl*, resource::LinkImpl*> link)
{
  private_links_.insert({position, link});
}

void ClusterZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  XBT_VERB("cluster getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());
  xbt_assert(not private_links_.empty(),
             "Cluster routing: no links attached to the source node - did you use host_link tag?");

  if ((src->id() == dst->id()) && has_loopback_) {
    if (src->is_router()) {
      XBT_WARN("Routing from a cluster private router to itself is meaningless");
    } else {
      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos(src->id()));
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->get_latency();
    }
    return;
  }

  if (not src->is_router()) { // No private link for the private router
    if (has_limiter_) {       // limiter for sender
      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(node_pos_with_loopback(src->id()));
      route->link_list.push_back(info.first);
    }

    std::pair<resource::LinkImpl*, resource::LinkImpl*> info =
        private_links_.at(node_pos_with_loopback_limiter(src->id()));
    if (info.first) { // link up
      route->link_list.push_back(info.first);
      if (lat)
        *lat += info.first->get_latency();
    }
  }

  if (backbone_) {
    route->link_list.push_back(backbone_);
    if (lat)
      *lat += backbone_->get_latency();
  }

  if (not dst->is_router()) { // No specific link for router
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info =
        private_links_.at(node_pos_with_loopback_limiter(dst->id()));

    if (info.second) { // link down
      route->link_list.push_back(info.second);
      if (lat)
        *lat += info.second->get_latency();
    }
    if (has_limiter_) { // limiter for receiver
      info = private_links_.at(node_pos_with_loopback(dst->id()));
      route->link_list.push_back(info.first);
    }
  }
}

void ClusterZone::get_graph(const s_xbt_graph_t* graph, std::map<std::string, xbt_node_t, std::less<>>* nodes,
                            std::map<std::string, xbt_edge_t, std::less<>>* edges)
{
  xbt_assert(router_,
             "Malformed cluster. This may be because your platform file is a hypergraph while it must be a graph.");

  /* create the router */
  xbt_node_t routerNode = new_xbt_graph_node(graph, router_->get_cname(), nodes);

  xbt_node_t backboneNode = nullptr;
  if (backbone_) {
    backboneNode = new_xbt_graph_node(graph, backbone_->get_cname(), nodes);
    new_xbt_graph_edge(graph, routerNode, backboneNode, edges);
  }

  for (auto const& src : get_vertices()) {
    if (not src->is_router()) {
      xbt_node_t previous = new_xbt_graph_node(graph, src->get_cname(), nodes);

      std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(src->id());

      if (info.first) { // link up
        xbt_node_t current = new_xbt_graph_node(graph, info.first->get_cname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }

      if (info.second) { // link down
        xbt_node_t current = new_xbt_graph_node(graph, info.second->get_cname(), nodes);
        new_xbt_graph_edge(graph, previous, current, edges);

        if (backbone_) {
          new_xbt_graph_edge(graph, current, backboneNode, edges);
        } else {
          new_xbt_graph_edge(graph, current, routerNode, edges);
        }
      }
    }
  }
}

void ClusterZone::create_links_for_node(const ClusterCreationArgs* cluster, int id, int /*rank*/, unsigned int position)
{
  std::string link_id = cluster->id + "_link_" + std::to_string(id);

  const s4u::Link* linkUp;
  const s4u::Link* linkDown;
  if (cluster->sharing_policy == simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX) {
    linkUp   = create_link(link_id + "_UP", std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();
    linkDown = create_link(link_id + "_DOWN", std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();
  } else {
    linkUp   = create_link(link_id, std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();
    linkDown = linkUp;
  }
  private_links_.insert({position, {linkUp->get_impl(), linkDown->get_impl()}});
}

void ClusterZone::set_gateway(unsigned int position, NetPoint* gateway)
{
  xbt_assert(not gateway || not gateway->is_netzone(), "ClusterZone: gateway cannot be another netzone %s",
             gateway->get_cname());
  gateways_[position] = gateway;
}

NetPoint* ClusterZone::get_gateway(unsigned int position)
{
  NetPoint* res = nullptr;
  auto it       = gateways_.find(position);
  if (it != gateways_.end()) {
    res = it->second;
  }
  return res;
}

void ClusterZone::fill_leaf_from_cb(unsigned int position, const std::vector<unsigned int>& dimensions,
                                    const s4u::ClusterCallbacks& set_callbacks, NetPoint** node_netpoint,
                                    s4u::Link** lb_link, s4u::Link** limiter_link)
{
  xbt_assert(node_netpoint, "Invalid node_netpoint parameter");
  xbt_assert(lb_link, "Invalid lb_link parameter");
  xbt_assert(limiter_link, "Invalid limiter_link paramater");
  *lb_link      = nullptr;
  *limiter_link = nullptr;

  // auxiliary function to get dims from index
  auto index_to_dims = [&dimensions](int index) {
    std::vector<unsigned int> dims_array(dimensions.size());
    for (unsigned long i = dimensions.size() - 1; i != 0; --i) {
      if (index <= 0) {
        break;
      }
      unsigned int value = index % dimensions[i];
      dims_array[i]      = value;
      index              = (index / dimensions[i]);
    }
    return dims_array;
  };

  kernel::routing::NetPoint* netpoint = nullptr;
  kernel::routing::NetPoint* gw       = nullptr;
  auto dims                           = index_to_dims(position);
  std::tie(netpoint, gw)              = set_callbacks.netpoint(get_iface(), dims, position);
  xbt_assert(netpoint, "set_netpoint(elem=%u): Invalid netpoint (nullptr)", position);
  if (netpoint->is_netzone()) {
    xbt_assert(gw && not gw->is_netzone(),
               "set_netpoint(elem=%u): Netpoint (%s) is a netzone, but gateway (%s) is invalid", position,
               netpoint->get_cname(), gw ? gw->get_cname() : "nullptr");
  } else {
    xbt_assert(not gw, "set_netpoint: Netpoint (%s) isn't netzone, gateway must be nullptr", netpoint->get_cname());
  }
  // setting gateway
  set_gateway(position, gw);

  if (set_callbacks.loopback) {
    s4u::Link* loopback = set_callbacks.loopback(get_iface(), dims, position);
    xbt_assert(loopback, "set_loopback: Invalid loopback link (nullptr) for element %u", position);
    set_loopback();
    add_private_link_at(node_pos(netpoint->id()), {loopback->get_impl(), loopback->get_impl()});
    *lb_link = loopback;
  }

  if (set_callbacks.limiter) {
    s4u::Link* limiter = set_callbacks.limiter(get_iface(), dims, position);
    xbt_assert(limiter, "set_limiter: Invalid limiter link (nullptr) for element %u", position);
    set_limiter();
    add_private_link_at(node_pos_with_loopback(netpoint->id()), {limiter->get_impl(), limiter->get_impl()});
    *limiter_link = limiter;
  }
  *node_netpoint = netpoint;
}

} // namespace routing
} // namespace kernel
} // namespace simgrid
