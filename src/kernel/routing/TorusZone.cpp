/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_torus, surf_route_cluster, "Torus Routing part of surf");

namespace simgrid {
namespace kernel {
namespace routing {
TorusZone::TorusZone(const std::string& name) : ClusterZone(name) {}

void TorusZone::create_links_for_node(ClusterCreationArgs* cluster, int id, int rank, unsigned int position)
{
  /* Create all links that exist in the torus. Each rank creates @a dimensions-1 links */
  int dim_product = 1; // Needed to calculate the next neighbor_id

  for (unsigned int j = 0; j < dimensions_.size(); j++) {
    LinkCreationArgs link;
    int current_dimension = dimensions_[j]; // which dimension are we currently in?
                                            // we need to iterate over all dimensions and create all links there
    // The other node the link connects
    int neighbor_rank_id = ((rank / dim_product) % current_dimension == current_dimension - 1)
                               ? rank - (current_dimension - 1) * dim_product
                               : rank + dim_product;
    // name of neighbor is not right for non contiguous cluster radicals (as id != rank in this case)
    std::string link_id =
        std::string(cluster->id) + "_link_from_" + std::to_string(id) + "_to_" + std::to_string(neighbor_rank_id);
    link.id = link_id;
    link.bandwidths.push_back(cluster->bw);
    link.latency = cluster->lat;
    link.policy  = cluster->sharing_policy;
    sg_platf_new_link(&link);
    resource::LinkImpl* linkUp;
    resource::LinkImpl* linkDown;
    if (link.policy == s4u::Link::SharingPolicy::SPLITDUPLEX) {
      linkUp   = s4u::Link::by_name(link_id + "_UP")->get_impl();
      linkDown = s4u::Link::by_name(link_id + "_DOWN")->get_impl();
    } else {
      linkUp   = s4u::Link::by_name(link_id)->get_impl();
      linkDown = linkUp;
    }
    /*
     * Add the link to its appropriate position.
     * Note that position rankId*(xbt_dynar_length(dimensions)+has_loopback?+has_limiter?)
     * holds the link "rankId->rankId"
     */
    private_links_.insert({position + j, {linkUp, linkDown}});
    dim_product *= current_dimension;
  }
}

void TorusZone::parse_specific_arguments(ClusterCreationArgs* cluster)
{
  std::vector<std::string> dimensions;
  boost::split(dimensions, cluster->topo_parameters, boost::is_any_of(","));

  if (not dimensions.empty()) {
    /* We are in a torus cluster
     * Parse attribute dimensions="dim1,dim2,dim3,...,dimN" and save them into a vector.
     * Additionally, we need to know how many ranks we have in total
     */
    for (auto const& group : dimensions)
      dimensions_.push_back(surf_parse_get_int(group));

    num_links_per_node_ = dimensions_.size();
  }
}

void TorusZone::get_local_route(NetPoint* src, NetPoint* dst, RouteCreationArgs* route, double* lat)
{
  XBT_VERB("torus getLocalRoute from '%s'[%u] to '%s'[%u]", src->get_cname(), src->id(), dst->get_cname(), dst->id());

  if (dst->is_router() || src->is_router())
    return;

  if (src->id() == dst->id() && has_loopback_) {
    std::pair<resource::LinkImpl*, resource::LinkImpl*> info = private_links_.at(src->id() * num_links_per_node_);

    route->link_list.push_back(info.first);
    if (lat)
      *lat += info.first->get_latency();
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
  int nodeOffset = (dsize + 1) * src->id();

  int linkOffset  = nodeOffset;
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
          nodeOffset = current_node * (num_links_per_node_);
          linkOffset = nodeOffset + (has_loopback_ ? 1 : 0) + (has_limiter_ ? 1 : 0) + j;
          use_lnk_up = true;
          assert(linkOffset >= 0);
        } else { // Route to the left
          if ((current_node / dim_product) % cur_dim == 0)
            next_node = (current_node - dim_product + dim_product * cur_dim);
          else
            next_node = (current_node - dim_product);

          // HERE: We use *next* node for calculation (as opposed to current_node!)
          nodeOffset = next_node * (num_links_per_node_);
          linkOffset = nodeOffset + j + (has_loopback_ ? 1 : 0) + (has_limiter_ ? 1 : 0);
          use_lnk_up = false;

          assert(linkOffset >= 0);
        }
        XBT_DEBUG("torus_get_route_and_latency - current_node: %u, next_node: %u, linkOffset is %i", current_node,
                  next_node, linkOffset);
        break;
      }

      dim_product *= cur_dim;
    }

    std::pair<resource::LinkImpl*, resource::LinkImpl*> info;

    if (has_limiter_) { // limiter for sender
      info = private_links_.at(nodeOffset + (has_loopback_ ? 1 : 0));
      route->link_list.push_back(info.first);
    }

    info                    = private_links_.at(linkOffset);
    resource::LinkImpl* lnk = use_lnk_up ? info.first : info.second;

    route->link_list.push_back(lnk);
    if (lat)
      *lat += lnk->get_latency();

    current_node = next_node;
  }
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
