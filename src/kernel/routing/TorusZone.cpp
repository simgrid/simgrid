/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <numeric>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_torus, surf_route_cluster, "Torus Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {

void TorusZone::create_links_for_node(ClusterCreationArgs* cluster, int id, int rank, unsigned int position)
{
  /* Create all links that exist in the torus. Each rank creates @a dimensions-1 links */
  int dim_product = 1; // Needed to calculate the next neighbor_id

  for (unsigned int j = 0; j < dimensions_.size(); j++) {
    int current_dimension = dimensions_[j]; // which dimension are we currently in?
                                            // we need to iterate over all dimensions and create all links there
    // The other node the link connects
    int neighbor_rank_id = ((rank / dim_product) % current_dimension == current_dimension - 1)
                               ? rank - (current_dimension - 1) * dim_product
                               : rank + dim_product;
    // name of neighbor is not right for non contiguous cluster radicals (as id != rank in this case)
    std::string link_id =
        std::string(cluster->id) + "_link_from_" + std::to_string(id) + "_to_" + std::to_string(neighbor_rank_id);
    const s4u::Link* linkup;
    const s4u::Link* linkdown;
    if (cluster->sharing_policy == s4u::Link::SharingPolicy::SPLITDUPLEX) {
      linkup   = create_link(link_id + "_UP", std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();
      linkdown = create_link(link_id + "_DOWN", std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();

    } else {
      linkup   = create_link(link_id, std::vector<double>{cluster->bw})->set_latency(cluster->lat)->seal();
      linkdown = linkup;
    }
    /*
     * Add the link to its appropriate position.
     * Note that position rankId*(xbt_dynar_length(dimensions)+has_loopback?+has_limiter?)
     * holds the link "rankId->rankId"
     */
    add_private_link_at(position + j, {linkup->get_impl(), linkdown->get_impl()});
    dim_product *= current_dimension;
  }
}

std::vector<unsigned int> TorusZone::parse_topo_parameters(const std::string& topo_parameters)
{
  std::vector<std::string> dimensions_str;
  boost::split(dimensions_str, topo_parameters, boost::is_any_of(","));
  std::vector<unsigned int> dimensions;

  if (not dimensions_str.empty()) {
    /* We are in a torus cluster
     * Parse attribute dimensions="dim1,dim2,dim3,...,dimN" and save them into a vector.
     * Additionally, we need to know how many ranks we have in total
     */
    std::transform(begin(dimensions_str), end(dimensions_str), std::back_inserter(dimensions), surf_parse_get_int);
  }
  return dimensions;
}

void TorusZone::parse_specific_arguments(ClusterCreationArgs* cluster)
{
  set_topology(TorusZone::parse_topo_parameters(cluster->topo_parameters));
}

void TorusZone::set_topology(const std::vector<unsigned int>& dimensions)
{
  xbt_assert(not dimensions.empty(), "Torus dimensions cannot be empty");
  dimensions_ = dimensions;
  set_num_links_per_node(dimensions_.size());
}

void TorusZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  XBT_VERB("torus getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  if (dst->is_router() || src->is_router())
    return;

  if (src->id() == dst->id() && has_loopback()) {
    resource::LinkImpl* uplink = get_uplink_from(node_pos(src->id()));

    route->link_list.push_back(uplink);
    if (lat)
      *lat += uplink->get_latency();
    return;
  }

  /*
   * Dimension based routing routes through each dimension consecutively
   * TODO Change to dynamic assignment
   */

  /*
   * Arrays that hold the coordinates of the current node and the target; comparing the values at the i-th position of
   * both arrays, we can easily assess whether we need to route into this dimension or not.
   */
  const unsigned int dsize = dimensions_.size();
  std::vector<unsigned int> myCoords(dsize);
  std::vector<unsigned int> targetCoords(dsize);
  unsigned int dim_size_product = 1;
  for (unsigned i = 0; i < dsize; i++) {
    unsigned cur_dim_size = dimensions_[i];
    myCoords[i]           = (src->id() / dim_size_product) % cur_dim_size;
    targetCoords[i]       = (dst->id() / dim_size_product) % cur_dim_size;
    dim_size_product *= cur_dim_size;
  }

  /*
   * linkOffset describes the offset where the link we want to use is stored(+1 is added because each node has a link
   * from itself to itself, which can only be the case if src->m_id == dst->m_id -- see above for this special case)
   */
  int linkOffset = (dsize + 1) * src->id();

  bool use_lnk_up = false; // Is this link of the form "cur -> next" or "next -> cur"? false means: next -> cur
  unsigned int current_node = src->id();
  while (current_node != dst->id()) {
    unsigned int next_node   = 0;
    unsigned int dim_product = 1; // First, we will route in x-dimension
    for (unsigned j = 0; j < dsize; j++) {
      const unsigned cur_dim = dimensions_[j];
      // current_node/dim_product = position in current dimension
      if ((current_node / dim_product) % cur_dim != (dst->id() / dim_product) % cur_dim) {
        if ((targetCoords[j] > myCoords[j] &&
             targetCoords[j] <= myCoords[j] + cur_dim / 2) // Is the target node on the right, without the wrap-around?
            ||
            (myCoords[j] > cur_dim / 2 && (myCoords[j] + cur_dim / 2) % cur_dim >=
                                              targetCoords[j])) { // Or do we need to use the wrap around to reach it?
          if ((current_node / dim_product) % cur_dim == cur_dim - 1)
            next_node = (current_node + dim_product - dim_product * cur_dim);
          else
            next_node = (current_node + dim_product);

          // HERE: We use *CURRENT* node for calculation (as opposed to next_node)
          linkOffset = node_pos_with_loopback_limiter(current_node) + j;
          use_lnk_up = true;
          assert(linkOffset >= 0);
        } else { // Route to the left
          if ((current_node / dim_product) % cur_dim == 0)
            next_node = (current_node - dim_product + dim_product * cur_dim);
          else
            next_node = (current_node - dim_product);

          // HERE: We use *next* node for calculation (as opposed to current_node!)
          linkOffset = node_pos_with_loopback_limiter(next_node) + j;
          use_lnk_up = false;

          assert(linkOffset >= 0);
        }
        XBT_DEBUG("torus_get_route_and_latency - current_node: %u, next_node: %u, linkOffset is %i", current_node,
                  next_node, linkOffset);
        break;
      }

      dim_product *= cur_dim;
    }

    if (has_limiter()) { // limiter for sender
      route->link_list.push_back(get_uplink_from(node_pos_with_loopback(current_node)));
    }

    resource::LinkImpl* lnk;
    if (use_lnk_up)
      lnk = get_uplink_from(linkOffset);
    else
      lnk = get_downlink_to(linkOffset);

    route->link_list.push_back(lnk);
    if (lat)
      *lat += lnk->get_latency();

    current_node = next_node;
  }
  // set gateways (if any)
  route->gw_src = get_gateway(src->id());
  route->gw_dst = get_gateway(dst->id());
}

/** @brief Auxiliary function to create hosts */
static std::pair<kernel::routing::NetPoint*, kernel::routing::NetPoint*>
create_torus_host(const kernel::routing::ClusterCreationArgs* cluster, s4u::NetZone* zone,
                  const std::vector<unsigned int>& /*coord*/, int id)
{
  std::string host_id = std::string(cluster->prefix) + std::to_string(id) + cluster->suffix;
  XBT_DEBUG("TorusCluster: creating host=%s speed=%f", host_id.c_str(), cluster->speeds.front());
  const s4u::Host* host = zone->create_host(host_id, cluster->speeds)
                              ->set_core_count(cluster->core_amount)
                              ->set_properties(cluster->properties)
                              ->seal();
  return std::make_pair(host->get_netpoint(), nullptr);
}

/** @brief Auxiliary function to create loopback links */
static s4u::Link* create_torus_loopback(const kernel::routing::ClusterCreationArgs* cluster, s4u::NetZone* zone,
                                        const std::vector<unsigned int>& /*coord*/, int id)
{
  std::string link_id = std::string(cluster->id) + "_link_" + std::to_string(id) + "_loopback";
  XBT_DEBUG("TorusCluster: creating loopback link=%s bw=%f", link_id.c_str(), cluster->loopback_bw);

  s4u::Link* loopback = zone->create_link(link_id, cluster->loopback_bw)
                            ->set_sharing_policy(simgrid::s4u::Link::SharingPolicy::FATPIPE)
                            ->set_latency(cluster->loopback_lat)
                            ->seal();
  return loopback;
}

/** @brief Auxiliary function to create limiter links */
static s4u::Link* create_torus_limiter(const kernel::routing::ClusterCreationArgs* cluster, s4u::NetZone* zone,
                                       const std::vector<unsigned int>& /*coord*/, int id)
{
  std::string link_id = std::string(cluster->id) + "_link_" + std::to_string(id) + "_limiter";
  XBT_DEBUG("TorusCluster: creating limiter link=%s bw=%f", link_id.c_str(), cluster->limiter_link);

  s4u::Link* limiter = zone->create_link(link_id, cluster->limiter_link)->seal();
  return limiter;
}

s4u::NetZone* create_torus_zone_with_hosts(const kernel::routing::ClusterCreationArgs* cluster,
                                           const s4u::NetZone* parent)
{
  using namespace std::placeholders;
  auto set_host = std::bind(create_torus_host, cluster, _1, _2, _3);
  std::function<s4u::TorusLinkCb> set_loopback{};
  std::function<s4u::TorusLinkCb> set_limiter{};

  if (cluster->loopback_bw > 0 || cluster->loopback_lat > 0) {
    set_loopback = std::bind(create_torus_loopback, cluster, _1, _2, _3);
  }

  if (cluster->limiter_link > 0) {
    set_loopback = std::bind(create_torus_limiter, cluster, _1, _2, _3);
  }

  return s4u::create_torus_zone(cluster->id, parent, TorusZone::parse_topo_parameters(cluster->topo_parameters),
                                cluster->bw, cluster->lat, cluster->sharing_policy, set_host, set_loopback,
                                set_limiter);
}

} // namespace routing
} // namespace kernel

namespace s4u {

NetZone* create_torus_zone(const std::string& name, const NetZone* parent, const std::vector<unsigned int>& dimensions,
                           double bandwidth, double latency, Link::SharingPolicy sharing_policy,
                           const std::function<TorusNetPointCb>& set_netpoint,
                           const std::function<TorusLinkCb>& set_loopback,
                           const std::function<TorusLinkCb>& set_limiter)
{
  int tot_elements = std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<>());
  if (dimensions.empty() || tot_elements <= 0)
    throw std::invalid_argument("TorusZone: incorrect dimensions parameter, each value must be > 0");
  if (bandwidth <= 0)
    throw std::invalid_argument("TorusZone: incorrect bandwidth for internode communication, bw=" +
                                std::to_string(bandwidth));
  if (latency < 0)
    throw std::invalid_argument("TorusZone: incorrect latency for internode communication, lat=" +
                                std::to_string(latency));

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

  auto* zone = new kernel::routing::TorusZone(name);
  zone->set_topology(dimensions);
  if (parent)
    zone->set_parent(parent->get_impl());

  for (int i = 0; i < tot_elements; i++) {
    kernel::routing::NetPoint* netpoint = nullptr;
    kernel::routing::NetPoint* gw       = nullptr;
    auto dims                           = index_to_dims(i);
    std::tie(netpoint, gw)              = set_netpoint(zone->get_iface(), dims, i);
    xbt_assert(netpoint, "TorusZone::set_netpoint(elem=%d): Invalid netpoint (nullptr)", i);
    if (netpoint->is_netzone()) {
      xbt_assert(gw && not gw->is_netzone(),
                 "TorusZone::set_netpoint(elem=%d): Netpoint (%s) is a netzone, but gateway (%s) is invalid", i,
                 netpoint->get_cname(), gw ? gw->get_cname() : "nullptr");
    } else {
      xbt_assert(not gw, "TorusZone: Netpoint (%s) isn't netzone, gateway must be nullptr", netpoint->get_cname());
    }
    // setting gateway
    zone->set_gateway(i, gw);

    if (set_loopback) {
      const Link* loopback = set_loopback(zone->get_iface(), dims, i);
      xbt_assert(loopback, "TorusZone::set_loopback: Invalid loopback link (nullptr) for element %d", i);
      zone->set_loopback();
      zone->add_private_link_at(zone->node_pos(netpoint->id()), {loopback->get_impl(), loopback->get_impl()});
    }

    if (set_limiter) {
      const Link* limiter = set_limiter(zone->get_iface(), dims, i);
      xbt_assert(limiter, "TorusZone::set_limiter: Invalid limiter link (nullptr) for element %d", i);
      zone->set_limiter();
      zone->add_private_link_at(zone->node_pos_with_loopback(netpoint->id()),
                                {limiter->get_impl(), limiter->get_impl()});
    }

    kernel::routing::ClusterCreationArgs params;
    params.id             = name;
    params.bw             = bandwidth;
    params.lat            = latency;
    params.sharing_policy = sharing_policy;
    zone->create_links_for_node(&params, netpoint->id(), i, zone->node_pos_with_loopback_limiter(netpoint->id()));
  }

  return zone->get_iface();
}
} // namespace s4u

} // namespace simgrid
