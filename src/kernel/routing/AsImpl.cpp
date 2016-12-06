/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/host.hpp"
#include "src/kernel/routing/AsImpl.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_route);

namespace simgrid {
  namespace kernel {
  namespace routing {

  AsImpl::AsImpl(As* father, const char* name) : As(father, name)
  {
    xbt_assert(nullptr == xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL),
               "Refusing to create a second AS called '%s'.", name);

    netcard_ = new NetCard(name, NetCard::Type::As, static_cast<AsImpl*>(father));
    xbt_lib_set(as_router_lib, name, ROUTING_ASR_LEVEL, static_cast<void*>(netcard_));
    XBT_DEBUG("AS '%s' created with the id '%d'", name, netcard_->id());
  }

  simgrid::s4u::Host* AsImpl::createHost(const char* name, std::vector<double>* speedPerPstate, int coreAmount)
  {
    simgrid::s4u::Host* res = new simgrid::s4u::Host(name);

    if (hierarchy_ == RoutingMode::unset)
      hierarchy_ = RoutingMode::base;

    res->pimpl_netcard = new NetCard(name, NetCard::Type::Host, this);

    surf_cpu_model_pm->createCpu(res, speedPerPstate, coreAmount);

    return res;
  }

  void AsImpl::getOneLinkRoutes(std::vector<Onelink*>* accumulator)
  {
    // recursing only. I have no route myself :)
    char* key;
    xbt_dict_cursor_t cursor = nullptr;
    AsImpl* rc_child;
    xbt_dict_foreach (children(), cursor, key, rc_child) {
      rc_child->getOneLinkRoutes(accumulator);
    }
  }

  /** @brief Get the common ancestor and its first children in each line leading to src and dst
   *
   * In the recursive case, this sets common_ancestor, src_ancestor and dst_ancestor are set as follows.
   * @verbatim
   *         platform root
   *               |
   *              ...                <- possibly long path
   *               |
   *         common_ancestor
   *           /        \
   *          /          \
   *         /            \          <- direct links
   *        /              \
   *       /                \
   *  src_ancestor     dst_ancestor  <- must be different in the recursive case
   *      |                   |
   *     ...                 ...     <-- possibly long pathes (one hop or more)
   *      |                   |
   *     src                 dst
   *  @endverbatim
   *
   *  In the base case (when src and dst are in the same AS), things are as follows:
   *  @verbatim
   *                  platform root
   *                        |
   *                       ...                      <-- possibly long path
   *                        |
   * common_ancestor==src_ancestor==dst_ancestor    <-- all the same value
   *                   /        \
   *                  /          \                  <-- direct links (exactly one hop)
   *                 /            \
   *              src              dst
   *  @endverbatim
   *
   * A specific recursive case occurs when src is the ancestor of dst. In this case,
   * the base case routing should be used so the common_ancestor is specifically set
   * to src_ancestor==dst_ancestor.
   * Naturally, things are completely symmetrical if dst is the ancestor of src.
   * @verbatim
   *            platform root
   *                  |
   *                 ...                <-- possibly long path
   *                  |
   *  src == src_ancestor==dst_ancestor==common_ancestor <-- same value
   *                  |
   *                 ...                <-- possibly long path (one hop or more)
   *                  |
   *                 dst
   *  @endverbatim
   */
  static void find_common_ancestors(NetCard* src, NetCard* dst,
                                    /* OUT */ AsImpl** common_ancestor, AsImpl** src_ancestor, AsImpl** dst_ancestor)
  {
    /* Deal with the easy base case */
    if (src->containingAS() == dst->containingAS()) {
      *common_ancestor = src->containingAS();
      *src_ancestor    = *common_ancestor;
      *dst_ancestor    = *common_ancestor;
      return;
    }

    /* engage the full recursive search */

    /* (1) find the path to root of src and dst*/
    AsImpl* src_as = src->containingAS();
    AsImpl* dst_as = dst->containingAS();

    xbt_assert(src_as, "Host %s must be in an AS", src->name().c_str());
    xbt_assert(dst_as, "Host %s must be in an AS", dst->name().c_str());

    /* (2) find the path to the root routing component */
    std::vector<AsImpl*> path_src;
    AsImpl* current = src->containingAS();
    while (current != nullptr) {
      path_src.push_back(current);
      current = static_cast<AsImpl*>(current->father());
    }
    std::vector<AsImpl*> path_dst;
    current = dst->containingAS();
    while (current != nullptr) {
      path_dst.push_back(current);
      current = static_cast<AsImpl*>(current->father());
    }

    /* (3) find the common father.
     * Before that, index_src and index_dst may be different, they both point to nullptr in path_src/path_dst
     * So we move them down simultaneously as long as they point to the same content.
     *
     * This works because all SimGrid platform have a unique root element (that is the last element of both paths).
     */
    AsImpl* father = nullptr; // the AS we dropped on the previous loop iteration
    while (path_src.size() > 1 && path_dst.size() > 1 &&
           path_src.at(path_src.size() - 1) == path_dst.at(path_dst.size() - 1)) {
      father = path_src.at(path_src.size() - 1);
      path_src.pop_back();
      path_dst.pop_back();
    }

    /* (4) we found the difference at least. Finalize the returned values */
    *src_ancestor = path_src.at(path_src.size() - 1); /* the first different father of src */
    *dst_ancestor = path_dst.at(path_dst.size() - 1); /* the first different father of dst */
    if (*src_ancestor == *dst_ancestor) {             // src is the ancestor of dst, or the contrary
      *common_ancestor = *src_ancestor;
    } else {
      *common_ancestor = father;
    }
  }

  /* PRECONDITION: this is the common ancestor of src and dst */
  bool AsImpl::getBypassRoute(routing::NetCard* src, routing::NetCard* dst,
                              /* OUT */ std::vector<surf::Link*>* links, double* latency)
  {
    // If never set a bypass route return nullptr without any further computations
    if (bypassRoutes_.empty())
      return false;

    /* Base case, no recursion is needed */
    if (dst->containingAS() == this && src->containingAS() == this) {
      if (bypassRoutes_.find({src, dst}) != bypassRoutes_.end()) {
        AsRoute* bypassedRoute = bypassRoutes_.at({src, dst});
        for (surf::Link* link : bypassedRoute->links) {
          links->push_back(link);
          if (latency)
            *latency += link->latency();
        }
        XBT_DEBUG("Found a bypass route from '%s' to '%s' with %zu links", src->cname(), dst->cname(),
                  bypassedRoute->links.size());
        return true;
      }
      return false;
    }

    /* Engage recursive search */


    /* (1) find the path to the root routing component */
    std::vector<AsImpl*> path_src;
    As* current = src->containingAS();
    while (current != nullptr) {
      path_src.push_back(static_cast<AsImpl*>(current));
      current = current->father_;
    }

    std::vector<AsImpl*> path_dst;
    current = dst->containingAS();
    while (current != nullptr) {
      path_dst.push_back(static_cast<AsImpl*>(current));
      current = current->father_;
    }

    /* (2) find the common father */
    while (path_src.size() > 1 && path_dst.size() > 1 &&
           path_src.at(path_src.size() - 1) == path_dst.at(path_dst.size() - 1)) {
      path_src.pop_back();
      path_dst.pop_back();
    }

    int max_index_src = path_src.size() - 1;
    int max_index_dst = path_dst.size() - 1;

    int max_index = std::max(max_index_src, max_index_dst);

    /* (3) Search for a bypass making the path up to the ancestor useless */
    AsRoute* bypassedRoute = nullptr;
    std::pair<kernel::routing::NetCard*, kernel::routing::NetCard*> key;
    for (int max = 0; max <= max_index; max++) {
      for (int i = 0; i < max; i++) {
        if (i <= max_index_src && max <= max_index_dst) {
          key = {path_src.at(i)->netcard_, path_dst.at(max)->netcard_};
          if (bypassRoutes_.find(key) != bypassRoutes_.end()) {
            bypassedRoute = bypassRoutes_.at(key);
            break;
          }
        }
        if (max <= max_index_src && i <= max_index_dst) {
          key = {path_src.at(max)->netcard_, path_dst.at(i)->netcard_};
          if (bypassRoutes_.find(key) != bypassRoutes_.end()) {
            bypassedRoute = bypassRoutes_.at(key);
            break;
          }
        }
      }

      if (bypassedRoute)
        break;

      if (max <= max_index_src && max <= max_index_dst) {
        key = {path_src.at(max)->netcard_, path_dst.at(max)->netcard_};
        if (bypassRoutes_.find(key) != bypassRoutes_.end()) {
          bypassedRoute = bypassRoutes_.at(key);
          break;
        }
      }
    }

    /* (4) If we have the bypass, use it. If not, caller will do the Right Thing. */
    if (bypassedRoute) {
      XBT_DEBUG("Found a bypass route from '%s' to '%s' with %zu links. We may have to complete it with recursive "
                "calls to getRoute",
                src->cname(), dst->cname(), bypassedRoute->links.size());
      if (src != key.first)
        getGlobalRoute(src, const_cast<NetCard*>(bypassedRoute->gw_src), links, latency);
      for (surf::Link* link : bypassedRoute->links) {
        links->push_back(link);
        if (latency)
          *latency += link->latency();
      }
      if (dst != key.second)
        getGlobalRoute(const_cast<NetCard*>(bypassedRoute->gw_dst), dst, links, latency);
      return true;
    }
    XBT_DEBUG("No bypass route from '%s' to '%s'.", src->cname(), dst->cname());
    return false;
    }

    void AsImpl::getGlobalRoute(routing::NetCard* src, routing::NetCard* dst,
                                /* OUT */ std::vector<surf::Link*>* links, double* latency)
    {
      s_sg_platf_route_cbarg_t route;
      memset(&route,0,sizeof(route));

      XBT_DEBUG("Resolve route from '%s' to '%s'", src->cname(), dst->cname());

      /* Find how src and dst are interconnected */
      AsImpl *common_ancestor, *src_ancestor, *dst_ancestor;
      find_common_ancestors(src, dst, &common_ancestor, &src_ancestor, &dst_ancestor);
      XBT_DEBUG("elements_father: common ancestor '%s' src ancestor '%s' dst ancestor '%s'",
          common_ancestor->name(), src_ancestor->name(), dst_ancestor->name());

      /* Check whether a direct bypass is defined. If so, use it and bail out */
      if (common_ancestor->getBypassRoute(src, dst, links, latency))
        return;

      /* If src and dst are in the same AS, life is good */
      if (src_ancestor == dst_ancestor) {       /* SURF_ROUTING_BASE */
        route.link_list = links;
        common_ancestor->getLocalRoute(src, dst, &route, latency);
        return;
      }

      /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

      route.link_list = new std::vector<surf::Link*>();

      common_ancestor->getLocalRoute(src_ancestor->netcard_, dst_ancestor->netcard_, &route, latency);
      xbt_assert((route.gw_src != nullptr) && (route.gw_dst != nullptr), "bad gateways for route from \"%s\" to \"%s\"",
                 src->name().c_str(), dst->name().c_str());

      /* If source gateway is not our source, we have to recursively find our way up to this point */
      if (src != route.gw_src)
        getGlobalRoute(src, route.gw_src, links, latency);
      for (auto link: *route.link_list)
        links->push_back(link);
      delete route.link_list;

      /* If dest gateway is not our destination, we have to recursively find our way from this point */
      if (route.gw_dst != dst)
        getGlobalRoute(route.gw_dst, dst, links, latency);
    }

}}} // namespace
