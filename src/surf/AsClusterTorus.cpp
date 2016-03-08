/* Copyright (c) 2014-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/AsClusterTorus.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_torus, surf_route_cluster, "Torus Routing part of surf");

inline unsigned int *rankId_to_coords(int rankId, xbt_dynar_t dimensions)
{

  unsigned int i = 0, cur_dim_size = 1, dim_size_product = 1;
  unsigned int *coords = (unsigned int *) malloc(xbt_dynar_length(dimensions) * sizeof(unsigned int));
  for (i = 0; i < xbt_dynar_length(dimensions); i++) {
    cur_dim_size = xbt_dynar_get_as(dimensions, i, int);
    coords[i] = (rankId / dim_size_product) % cur_dim_size;
    dim_size_product *= cur_dim_size;
  }

  return coords;
}


namespace simgrid {
  namespace surf {
    AsClusterTorus::AsClusterTorus(const char*name)
      : AsCluster(name) {
    }
    AsClusterTorus::~AsClusterTorus() {
      xbt_dynar_free(&dimensions_);
    }

    void AsClusterTorus::create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position) {
      s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
      char *link_id;
      unsigned int j = 0;
      /**
       * Create all links that exist in the torus.
       * Each rank creates #dimensions-1 links
       */
      int neighbour_rank_id = 0;        // The other node the link connects
      int current_dimension = 0,        // which dimension are we currently in?
          // we need to iterate over all dimensions
          // and create all links there
          dim_product = 1;      // Needed to calculate the next neighbour_id
      for (j = 0; j < xbt_dynar_length(dimensions_); j++) {

        memset(&link, 0, sizeof(link));
        current_dimension = xbt_dynar_get_as(dimensions_, j, int);
        neighbour_rank_id =
            (((int) rank / dim_product) % current_dimension ==
                current_dimension - 1) ? rank - (current_dimension - 1) * dim_product : rank + dim_product;
        //name of neighbor is not right for non contiguous cluster radicals (as id != rank in this case)
        link_id = bprintf("%s_link_from_%i_to_%i", cluster->id, id, neighbour_rank_id);
        link.id = link_id;
        link.bandwidth = cluster->bw;
        link.latency = cluster->lat;
        link.policy = cluster->sharing_policy;
        sg_platf_new_link(&link);
        s_surf_parsing_link_up_down_t info;
        if (link.policy == SURF_LINK_FULLDUPLEX) {
          char *tmp_link = bprintf("%s_UP", link_id);
          info.link_up = Link::byName(tmp_link);
          free(tmp_link);
          tmp_link = bprintf("%s_DOWN", link_id);
          info.link_down = Link::byName(tmp_link);
          free(tmp_link);
        } else {
          info.link_up = Link::byName(link_id);
          info.link_down = info.link_up;
        }
        /**
         * Add the link to its appropriate position;
         * note that position rankId*(xbt_dynar_length(dimensions)+has_loopack?+has_limiter?)
         * holds the link "rankId->rankId"
         */
        xbt_dynar_set(upDownLinks, position + j, &info);
        dim_product *= current_dimension;
        xbt_free(link_id);
      }
      rank++;
    }

    void AsClusterTorus::parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) {

      unsigned int iter;
      char *groups;
      xbt_dynar_t dimensions = xbt_str_split(cluster->topo_parameters, ",");

      if (!xbt_dynar_is_empty(dimensions)) {
        dimensions_ = xbt_dynar_new(sizeof(int), NULL);
        /**
         * We are in a torus cluster
         * Parse attribute dimensions="dim1,dim2,dim3,...,dimN"
         * and safe it in a dynarray.
         * Additionally, we need to know how many ranks we have in total
         */
        xbt_dynar_foreach(dimensions, iter, groups) {
          int tmp = surf_parse_get_int(xbt_dynar_get_as(dimensions, iter, char *));
          xbt_dynar_set_as(dimensions_, iter, int, tmp);
        }

        nb_links_per_node_ = xbt_dynar_length(dimensions_);

      }
      xbt_dynar_free(&dimensions);
    }

    void AsClusterTorus::getRouteAndLatency(NetCard * src, NetCard * dst, sg_platf_route_cbarg_t route, double *lat) {

      XBT_VERB("torus_get_route_and_latency from '%s'[%d] to '%s'[%d]",
          src->name(), src->id(), dst->name(), dst->id());

      if (dst->isRouter() || src->isRouter())
        return;

      if ((src->id() == dst->id()) && has_loopback_) {
        s_surf_parsing_link_up_down_t info = xbt_dynar_get_as(upDownLinks, src->id() * nb_links_per_node_, s_surf_parsing_link_up_down_t);

        route->link_list->push_back(info.link_up);
        if (lat)
          *lat += info.link_up->getLatency();
        return;
      }


      /**
       * Dimension based routing routes through each dimension consecutively
       * TODO Change to dynamic assignment
       */
      unsigned int j, cur_dim, dim_product = 1;
      int current_node = src->id();
      int unsigned next_node = 0;
      /**
       * Arrays that hold the coordinates of the current node and
       * the target; comparing the values at the i-th position of
       * both arrays, we can easily assess whether we need to route
       * into this dimension or not.
       */
      unsigned int *myCoords, *targetCoords;
      myCoords = rankId_to_coords(src->id(), dimensions_);
      targetCoords = rankId_to_coords(dst->id(), dimensions_);
      /**
       * linkOffset describes the offset where the link
       * we want to use is stored
       * (+1 is added because each node has a link from itself to itself,
       * which can only be the case if src->m_id == dst->m_id -- see above
       * for this special case)
       */
      int nodeOffset = (xbt_dynar_length(dimensions_) + 1) * src->id();

      int linkOffset = nodeOffset;
      bool use_lnk_up = false;  // Is this link of the form "cur -> next" or "next -> cur"?
      // false means: next -> cur
      while (current_node != dst->id()) {
        dim_product = 1;        // First, we will route in x-dimension
        for (j = 0; j < xbt_dynar_length(dimensions_); j++) {
          cur_dim = xbt_dynar_get_as(dimensions_, j, int);

          // current_node/dim_product = position in current dimension
          if ((current_node / dim_product) % cur_dim != (dst->id() / dim_product) % cur_dim) {

            if ((targetCoords[j] > myCoords[j] && targetCoords[j] <= myCoords[j] + cur_dim / 2) // Is the target node on the right, without the wrap-around?
                || (myCoords[j] > cur_dim / 2 && (myCoords[j] + cur_dim / 2) % cur_dim >= targetCoords[j])) {   // Or do we need to use the wrap around to reach it?
              if ((current_node / dim_product) % cur_dim == cur_dim - 1)
                next_node = (current_node + dim_product - dim_product * cur_dim);
              else
                next_node = (current_node + dim_product);

              // HERE: We use *CURRENT* node for calculation (as opposed to next_node)
              nodeOffset = current_node * (nb_links_per_node_);
              linkOffset = nodeOffset + has_loopback_ + has_limiter_ + j;
              use_lnk_up = true;
              assert(linkOffset >= 0);
            } else {            // Route to the left
              if ((current_node / dim_product) % cur_dim == 0)
                next_node = (current_node - dim_product + dim_product * cur_dim);
              else
                next_node = (current_node - dim_product);

              // HERE: We use *next* node for calculation (as opposed to current_node!)
              nodeOffset = next_node * (nb_links_per_node_);
              linkOffset = nodeOffset + j + has_loopback_ + has_limiter_;
              use_lnk_up = false;

              assert(linkOffset >= 0);
            }
            XBT_DEBUG("torus_get_route_and_latency - current_node: %i, next_node: %u, linkOffset is %i",
                current_node, next_node, linkOffset);

            break;
          }

          dim_product *= cur_dim;
        }

        s_surf_parsing_link_up_down_t info;

        if (has_limiter_) {    // limiter for sender
          info = xbt_dynar_get_as(upDownLinks, nodeOffset + has_loopback_, s_surf_parsing_link_up_down_t);
          route->link_list->push_back(info.link_up);
        }

        info = xbt_dynar_get_as(upDownLinks, linkOffset, s_surf_parsing_link_up_down_t);

        if (use_lnk_up == false) {
          route->link_list->push_back(info.link_down);
          if (lat)
            *lat += info.link_down->getLatency();
        } else {
          route->link_list->push_back(info.link_up);
          if (lat)
            *lat += info.link_up->getLatency();
        }
        current_node = next_node;
        next_node = 0;
      }
      free(myCoords);
      free(targetCoords);

      return;
    }

  }
}
