/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "src/surf/AsImpl.hpp"
#include "src/surf/network_interface.hpp" // Link FIXME: move to proper header

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(AsImpl,surf, "Implementation of S4U autonomous systems");

namespace simgrid {
  namespace surf {

    AsImpl::AsImpl(const char *name)
    : As(name)
    {
    }
    AsImpl::~AsImpl()
    {
      xbt_dynar_free(&upDownLinks);
      delete netcard_;
    }

    xbt_dynar_t AsImpl::getOneLinkRoutes() {
      return NULL;
    }

    /** @brief Get the common ancestor and its first childs in each line leading to src and dst */
    static void find_common_ancestors(NetCard *src, NetCard *dst,
        /* OUT */ AsImpl **common_ancestor, AsImpl **src_ancestor, AsImpl **dst_ancestor)
    {
    #define ROUTING_HIERARCHY_MAXDEPTH 32     /* increase if it is not enough */
      AsImpl *path_src[ROUTING_HIERARCHY_MAXDEPTH];
      AsImpl *path_dst[ROUTING_HIERARCHY_MAXDEPTH];
      int index_src = 0;
      int index_dst = 0;
      AsImpl *current_src;
      AsImpl *current_dst;
      AsImpl *father;

      /* (1) find the path to root of src and dst*/
      AsImpl *src_as = src->containingAS();
      AsImpl *dst_as = dst->containingAS();

      xbt_assert(src_as, "Host %s must be in an AS", src->name());
      xbt_assert(dst_as, "Host %s must be in an AS", dst->name());

      /* (2) find the path to the root routing component */
      for (AsImpl *current = src_as; current != NULL; current = static_cast<AsImpl*>(current->father())) {
        xbt_assert(index_src < ROUTING_HIERARCHY_MAXDEPTH, "ROUTING_HIERARCHY_MAXDEPTH should be increased for element %s", src->name());
        path_src[index_src++] = current;
      }
      for (AsImpl *current = dst_as; current != NULL; current = static_cast<AsImpl*>(current->father())) {
        xbt_assert(index_dst < ROUTING_HIERARCHY_MAXDEPTH,"ROUTING_HIERARCHY_MAXDEPTH should be increased for path_dst");
        path_dst[index_dst++] = current;
      }

      /* (3) find the common father.
       * Before that, index_src and index_dst may be different, they both point to NULL in path_src/path_dst
       * So we move them down simultaneously as long as they point to the same content.
       */
      do {
        current_src = path_src[--index_src];
        current_dst = path_dst[--index_dst];
      } while (index_src > 0 && index_dst > 0 && current_src == current_dst);

      /* (4) if we did not find a difference (index_src or index_dst went to 0), both elements are in the same AS */
      if (current_src == current_dst)
        father = current_src;
      else // we found a difference
        father = path_src[index_src + 1];

      /* (5) result generation */
      *common_ancestor = father;    /* the common father of src and dst */
      *src_ancestor = current_src;  /* the first different father of src */
      *dst_ancestor = current_dst;  /* the first different father of dst */
    #undef ROUTING_HIERARCHY_MAXDEPTH
    }


    /* PRECONDITION: this is the common ancestor of src and dst */
    std::vector<surf::Link*> *AsImpl::getBypassRoute(surf::NetCard *src, surf::NetCard *dst)
    {
      // If never set a bypass route return NULL without any further computations
      XBT_DEBUG("generic_get_bypassroute from %s to %s", src->name(), dst->name());
      if (bypassRoutes_.empty())
        return nullptr;

      std::vector<surf::Link*> *bypassedRoute = nullptr;

      if(dst->containingAS() == this && src->containingAS() == this ){
        if (bypassRoutes_.find({src->name(),dst->name()}) != bypassRoutes_.end()) {
          bypassedRoute = bypassRoutes_.at({src->name(),dst->name()});
          XBT_DEBUG("Found a bypass route with %zu links",bypassedRoute->size());
        }
        return bypassedRoute;
      }

      /* (2) find the path to the root routing component */
      std::vector<As*> path_src;
      As *current = src->containingAS();
      while (current != NULL) {
        path_src.push_back(current);
        current = current->father_;
      }

      std::vector<As*> path_dst;
      current = dst->containingAS();
      while (current != NULL) {
        path_dst.push_back(current);
        current = current->father_;
      }

      /* (3) find the common father */
      while (path_src.size() > 1 && path_dst.size() >1
          && path_src.at(path_src.size() -1) == path_dst.at(path_dst.size() -1)) {
        path_src.pop_back();
        path_dst.pop_back();
      }

      int max_index_src = path_src.size() - 1;
      int max_index_dst = path_dst.size() - 1;

      int max_index = std::max(max_index_src, max_index_dst);

      for (int max = 0; max <= max_index; max++) {
        for (int i = 0; i < max; i++) {
          if (i <= max_index_src && max <= max_index_dst) {
            const std::pair<std::string, std::string> key = {path_src.at(i)->name(), path_dst.at(max)->name()};
            if (bypassRoutes_.find(key) != bypassRoutes_.end())
              bypassedRoute = bypassRoutes_.at(key);
          }
          if (bypassedRoute)
            break;
          if (max <= max_index_src && i <= max_index_dst) {
            const std::pair<std::string, std::string> key = {path_src.at(max)->name(), path_dst.at(i)->name()};
            if (bypassRoutes_.find(key) != bypassRoutes_.end())
              bypassedRoute = bypassRoutes_.at(key);
          }
          if (bypassedRoute)
            break;
        }

        if (bypassedRoute)
          break;

        if (max <= max_index_src && max <= max_index_dst) {
          const std::pair<std::string, std::string> key = {path_src.at(max)->name(), path_dst.at(max)->name()};
          if (bypassRoutes_.find(key) != bypassRoutes_.end())
            bypassedRoute = bypassRoutes_.at(key);
        }
        if (bypassedRoute)
          break;
      }

      return bypassedRoute;
    }

    /**
     * \brief Recursive function for getRouteAndLatency
     *
     * \param src the source host
     * \param dst the destination host
     * \param links Where to store the links and the gw information
     * \param latency If not NULL, the latency of all links will be added in it
     */
    void AsImpl::getRouteRecursive(surf::NetCard *src, surf::NetCard *dst,
        /* OUT */ std::vector<surf::Link*> * links, double *latency)
    {
      s_sg_platf_route_cbarg_t route;
      memset(&route,0,sizeof(route));

      XBT_DEBUG("Solve route/latency \"%s\" to \"%s\"", src->name(), dst->name());

      /* Find how src and dst are interconnected */
      AsImpl *common_ancestor, *src_ancestor, *dst_ancestor;
      find_common_ancestors(src, dst, &common_ancestor, &src_ancestor, &dst_ancestor);
      XBT_DEBUG("elements_father: common ancestor '%s' src ancestor '%s' dst ancestor '%s'",
          common_ancestor->name(), src_ancestor->name(), dst_ancestor->name());

      /* Check whether a direct bypass is defined. If so, use it and bail out */
      std::vector<surf::Link*> *bypassed_route = common_ancestor->getBypassRoute(src, dst);
      if (nullptr != bypassed_route) {
        for (surf::Link *link : *bypassed_route) {
          links->push_back(link);
          if (latency)
            *latency += link->getLatency();
        }
        return;
      }

      /* If src and dst are in the same AS, life is good */
      if (src_ancestor == dst_ancestor) {       /* SURF_ROUTING_BASE */
        route.link_list = links;
        common_ancestor->getRouteAndLatency(src, dst, &route, latency);
        return;
      }

      /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

      route.link_list = new std::vector<surf::Link*>();

      common_ancestor->getRouteAndLatency(src_ancestor->netcard_, dst_ancestor->netcard_, &route, latency);
      xbt_assert((route.gw_src != NULL) && (route.gw_dst != NULL),
          "bad gateways for route from \"%s\" to \"%s\"", src->name(), dst->name());

      /* If source gateway is not our source, we have to recursively find our way up to this point */
      if (src != route.gw_src)
        getRouteRecursive(src, route.gw_src, links, latency);
      for (auto link: *route.link_list)
        links->push_back(link);

      /* If dest gateway is not our destination, we have to recursively find our way from this point */
      if (route.gw_dst != dst)
        getRouteRecursive(route.gw_dst, dst, links, latency);

    }

  }
};
