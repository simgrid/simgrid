/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/host.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_route);

namespace simgrid {
namespace kernel {
namespace routing {

class BypassRoute {
public:
  explicit BypassRoute(NetCard* gwSrc, NetCard* gwDst) : gw_src(gwSrc), gw_dst(gwDst) {}
  const NetCard* gw_src;
  const NetCard* gw_dst;
  std::vector<Link*> links;
};

NetZoneImpl::NetZoneImpl(NetZone* father, const char* name) : NetZone(father, name)
{
  xbt_assert(nullptr == xbt_dict_get_or_null(netcards_dict, name), "Refusing to create a second NetZone called '%s'.",
             name);

  netcard_ = new NetCard(name, NetCard::Type::NetZone, static_cast<NetZoneImpl*>(father));
  xbt_dict_set(netcards_dict, name, static_cast<void*>(netcard_), nullptr);
  XBT_DEBUG("NetZone '%s' created with the id '%d'", name, netcard_->id());
}
NetZoneImpl::~NetZoneImpl()
{
  for (auto& kv : bypassRoutes_)
    delete kv.second;
}

simgrid::s4u::Host* NetZoneImpl::createHost(const char* name, std::vector<double>* speedPerPstate, int coreAmount)
{
  simgrid::s4u::Host* res = new simgrid::s4u::Host(name);

  if (hierarchy_ == RoutingMode::unset)
    hierarchy_ = RoutingMode::base;

  res->pimpl_netcard = new NetCard(name, NetCard::Type::Host, this);

  surf_cpu_model_pm->createCpu(res, speedPerPstate, coreAmount);

  return res;
}

void NetZoneImpl::addBypassRoute(sg_platf_route_cbarg_t e_route)
{
  /* Argument validity checks */
  if (e_route->gw_dst) {
    XBT_DEBUG("Load bypassASroute from %s@%s to %s@%s", e_route->src->cname(), e_route->gw_src->cname(),
              e_route->dst->cname(), e_route->gw_dst->cname());
    xbt_assert(!e_route->link_list->empty(), "Bypass route between %s@%s and %s@%s cannot be empty.",
               e_route->src->cname(), e_route->gw_src->cname(), e_route->dst->cname(), e_route->gw_dst->cname());
    xbt_assert(bypassRoutes_.find({e_route->src, e_route->dst}) == bypassRoutes_.end(),
               "The bypass route between %s@%s and %s@%s already exists.", e_route->src->cname(),
               e_route->gw_src->cname(), e_route->dst->cname(), e_route->gw_dst->cname());
  } else {
    XBT_DEBUG("Load bypassRoute from %s to %s", e_route->src->cname(), e_route->dst->cname());
    xbt_assert(!e_route->link_list->empty(), "Bypass route between %s and %s cannot be empty.", e_route->src->cname(),
               e_route->dst->cname());
    xbt_assert(bypassRoutes_.find({e_route->src, e_route->dst}) == bypassRoutes_.end(),
               "The bypass route between %s and %s already exists.", e_route->src->cname(), e_route->dst->cname());
  }

  /* Build a copy that will be stored in the dict */
  kernel::routing::BypassRoute* newRoute = new kernel::routing::BypassRoute(e_route->gw_src, e_route->gw_dst);
  for (auto link : *e_route->link_list)
    newRoute->links.push_back(link);

  /* Store it */
  bypassRoutes_.insert({{e_route->src, e_route->dst}, newRoute});
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
 *     ...                 ...     <-- possibly long paths (one hop or more)
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
                                  /* OUT */ NetZoneImpl** common_ancestor, NetZoneImpl** src_ancestor,
                                  NetZoneImpl** dst_ancestor)
{
  /* Deal with the easy base case */
  if (src->netzone() == dst->netzone()) {
    *common_ancestor = src->netzone();
    *src_ancestor    = *common_ancestor;
    *dst_ancestor    = *common_ancestor;
    return;
  }

  /* engage the full recursive search */

  /* (1) find the path to root of src and dst*/
  NetZoneImpl* src_as = src->netzone();
  NetZoneImpl* dst_as = dst->netzone();

  xbt_assert(src_as, "Host %s must be in an AS", src->cname());
  xbt_assert(dst_as, "Host %s must be in an AS", dst->cname());

  /* (2) find the path to the root routing component */
  std::vector<NetZoneImpl*> path_src;
  NetZoneImpl* current = src->netzone();
  while (current != nullptr) {
    path_src.push_back(current);
    current = static_cast<NetZoneImpl*>(current->father());
  }
  std::vector<NetZoneImpl*> path_dst;
  current = dst->netzone();
  while (current != nullptr) {
    path_dst.push_back(current);
    current = static_cast<NetZoneImpl*>(current->father());
  }

  /* (3) find the common father.
   * Before that, index_src and index_dst may be different, they both point to nullptr in path_src/path_dst
   * So we move them down simultaneously as long as they point to the same content.
   *
   * This works because all SimGrid platform have a unique root element (that is the last element of both paths).
   */
  NetZoneImpl* father = nullptr; // the netzone we dropped on the previous loop iteration
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
bool NetZoneImpl::getBypassRoute(routing::NetCard* src, routing::NetCard* dst,
                                 /* OUT */ std::vector<surf::Link*>* links, double* latency)
{
  // If never set a bypass route return nullptr without any further computations
  if (bypassRoutes_.empty())
    return false;

  /* Base case, no recursion is needed */
  if (dst->netzone() == this && src->netzone() == this) {
    if (bypassRoutes_.find({src, dst}) != bypassRoutes_.end()) {
      BypassRoute* bypassedRoute = bypassRoutes_.at({src, dst});
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
  std::vector<NetZoneImpl*> path_src;
  NetZone* current = src->netzone();
  while (current != nullptr) {
    path_src.push_back(static_cast<NetZoneImpl*>(current));
    current = current->father_;
  }

  std::vector<NetZoneImpl*> path_dst;
  current = dst->netzone();
  while (current != nullptr) {
    path_dst.push_back(static_cast<NetZoneImpl*>(current));
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
  BypassRoute* bypassedRoute = nullptr;
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

void NetZoneImpl::getGlobalRoute(routing::NetCard* src, routing::NetCard* dst,
                                 /* OUT */ std::vector<surf::Link*>* links, double* latency)
{
  s_sg_platf_route_cbarg_t route;
  memset(&route, 0, sizeof(route));

  XBT_DEBUG("Resolve route from '%s' to '%s'", src->cname(), dst->cname());

  /* Find how src and dst are interconnected */
  NetZoneImpl *common_ancestor, *src_ancestor, *dst_ancestor;
  find_common_ancestors(src, dst, &common_ancestor, &src_ancestor, &dst_ancestor);
  XBT_DEBUG("elements_father: common ancestor '%s' src ancestor '%s' dst ancestor '%s'", common_ancestor->name(),
            src_ancestor->name(), dst_ancestor->name());

  /* Check whether a direct bypass is defined. If so, use it and bail out */
  if (common_ancestor->getBypassRoute(src, dst, links, latency))
    return;

  /* If src and dst are in the same AS, life is good */
  if (src_ancestor == dst_ancestor) { /* SURF_ROUTING_BASE */
    route.link_list = links;
    common_ancestor->getLocalRoute(src, dst, &route, latency);
    return;
  }

  /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

  route.link_list = new std::vector<surf::Link*>();

  common_ancestor->getLocalRoute(src_ancestor->netcard_, dst_ancestor->netcard_, &route, latency);
  xbt_assert((route.gw_src != nullptr) && (route.gw_dst != nullptr), "bad gateways for route from \"%s\" to \"%s\"",
             src->cname(), dst->cname());

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != route.gw_src)
    getGlobalRoute(src, route.gw_src, links, latency);
  for (auto link : *route.link_list)
    links->push_back(link);
  delete route.link_list;

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  if (route.gw_dst != dst)
    getGlobalRoute(route.gw_dst, dst, links, latency);
}
}
}
} // namespace
