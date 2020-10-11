/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_route);

namespace simgrid {
namespace kernel {
namespace routing {

NetZoneImpl::NetZoneImpl(NetZoneImpl* father, const std::string& name, resource::NetworkModel* network_model)
    : piface_(this), father_(father), name_(name), network_model_(network_model)
{
  xbt_assert(nullptr == s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()),
             "Refusing to create a second NetZone called '%s'.", get_cname());

  netpoint_ = new NetPoint(name_, NetPoint::Type::NetZone, father_);
  XBT_DEBUG("NetZone '%s' created with the id '%u'", get_cname(), netpoint_->id());
}

NetZoneImpl::~NetZoneImpl()
{
  for (auto const& nz : children_)
    delete nz;

  for (auto const& kv : bypass_routes_)
    delete kv.second;

  s4u::Engine::get_instance()->netpoint_unregister(netpoint_);
}

/** @brief Returns the list of the hosts found in this NetZone (not recursively)
 *
 * Only the hosts that are directly contained in this NetZone are retrieved,
 * not the ones contained in sub-netzones.
 */
std::vector<s4u::Host*> NetZoneImpl::get_all_hosts() const
{
  std::vector<s4u::Host*> res;
  for (auto const& card : get_vertices()) {
    s4u::Host* host = s4u::Host::by_name_or_null(card->get_name());
    if (host != nullptr)
      res.push_back(host);
  }
  return res;
}
int NetZoneImpl::get_host_count() const
{
  int count = 0;
  for (auto const& card : get_vertices()) {
    const s4u::Host* host = s4u::Host::by_name_or_null(card->get_name());
    if (host != nullptr)
      count++;
  }
  return count;
}

s4u::Link* NetZoneImpl::create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                                    s4u::Link::SharingPolicy policy,
                                    const std::unordered_map<std::string, std::string>* props)
{
  static double last_warned_latency = sg_surf_precision;
  if (latency != 0.0 && latency < last_warned_latency) {
    XBT_WARN("Latency for link %s is smaller than surf/precision (%g < %g)."
             " For more accuracy, consider setting \"--cfg=surf/precision:%g\".",
             name.c_str(), latency, sg_surf_precision, latency);
    last_warned_latency = latency;
  }

  auto* l = surf_network_model->create_link(name, bandwidths, latency, policy);

  if (props)
    l->set_properties(*props);

  return l->get_iface();
}
s4u::Host* NetZoneImpl::create_host(const std::string& name, const std::vector<double>& speed_per_pstate,
                                    int coreAmount, const std::unordered_map<std::string, std::string>* props)
{
  auto* res = new s4u::Host(name);

  if (hierarchy_ == RoutingMode::unset)
    hierarchy_ = RoutingMode::base;

  res->set_netpoint(new NetPoint(name, NetPoint::Type::Host, this));

  surf_cpu_model_pm->create_cpu(res, speed_per_pstate, coreAmount);

  if (props != nullptr)
    res->set_properties(*props);

  s4u::Host::on_creation(*res); // notify the signal

  return res;
}

int NetZoneImpl::add_component(NetPoint* elm)
{
  vertices_.push_back(elm);
  return vertices_.size() - 1; // The rank of the newly created object
}

void NetZoneImpl::add_route(NetPoint* /*src*/, NetPoint* /*dst*/, NetPoint* /*gw_src*/, NetPoint* /*gw_dst*/,
                            std::vector<resource::LinkImpl*>& /*link_list*/, bool /*symmetrical*/)
{
  xbt_die("NetZone '%s' does not accept new routes (wrong class).", get_cname());
}

void NetZoneImpl::add_bypass_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                   std::vector<resource::LinkImpl*>& link_list, bool /* symmetrical */)
{
  /* Argument validity checks */
  if (gw_dst) {
    XBT_DEBUG("Load bypassNetzoneRoute from %s@%s to %s@%s", src->get_cname(), gw_src->get_cname(), dst->get_cname(),
              gw_dst->get_cname());
    xbt_assert(not link_list.empty(), "Bypass route between %s@%s and %s@%s cannot be empty.", src->get_cname(),
               gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
    xbt_assert(bypass_routes_.find({src, dst}) == bypass_routes_.end(),
               "The bypass route between %s@%s and %s@%s already exists.", src->get_cname(), gw_src->get_cname(),
               dst->get_cname(), gw_dst->get_cname());
  } else {
    XBT_DEBUG("Load bypassRoute from %s to %s", src->get_cname(), dst->get_cname());
    xbt_assert(not link_list.empty(), "Bypass route between %s and %s cannot be empty.", src->get_cname(),
               dst->get_cname());
    xbt_assert(bypass_routes_.find({src, dst}) == bypass_routes_.end(),
               "The bypass route between %s and %s already exists.", src->get_cname(), dst->get_cname());
  }

  /* Build a copy that will be stored in the dict */
  auto* newRoute = new BypassRoute(gw_src, gw_dst);
  for (auto const& link : link_list)
    newRoute->links.push_back(link);

  /* Store it */
  bypass_routes_.insert({{src, dst}, newRoute});
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
 *  In the base case (when src and dst are in the same netzone), things are as follows:
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
static void find_common_ancestors(NetPoint* src, NetPoint* dst,
                                  /* OUT */ NetZoneImpl** common_ancestor, NetZoneImpl** src_ancestor,
                                  NetZoneImpl** dst_ancestor)
{
  /* Deal with the easy base case */
  if (src->get_englobing_zone() == dst->get_englobing_zone()) {
    *common_ancestor = src->get_englobing_zone();
    *src_ancestor    = *common_ancestor;
    *dst_ancestor    = *common_ancestor;
    return;
  }

  /* engage the full recursive search */

  /* (1) find the path to root of src and dst*/
  const NetZoneImpl* src_as = src->get_englobing_zone();
  const NetZoneImpl* dst_as = dst->get_englobing_zone();

  xbt_assert(src_as, "Host %s must be in a netzone", src->get_cname());
  xbt_assert(dst_as, "Host %s must be in a netzone", dst->get_cname());

  /* (2) find the path to the root routing component */
  std::vector<NetZoneImpl*> path_src;
  NetZoneImpl* current = src->get_englobing_zone();
  while (current != nullptr) {
    path_src.push_back(current);
    current = current->get_father();
  }
  std::vector<NetZoneImpl*> path_dst;
  current = dst->get_englobing_zone();
  while (current != nullptr) {
    path_dst.push_back(current);
    current = current->get_father();
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
bool NetZoneImpl::get_bypass_route(NetPoint* src, NetPoint* dst,
                                   /* OUT */ std::vector<resource::LinkImpl*>& links, double* latency)
{
  // If never set a bypass route return nullptr without any further computations
  if (bypass_routes_.empty())
    return false;

  /* Base case, no recursion is needed */
  if (dst->get_englobing_zone() == this && src->get_englobing_zone() == this) {
    if (bypass_routes_.find({src, dst}) != bypass_routes_.end()) {
      const BypassRoute* bypassedRoute = bypass_routes_.at({src, dst});
      for (resource::LinkImpl* const& link : bypassedRoute->links) {
        links.push_back(link);
        if (latency)
          *latency += link->get_latency();
      }
      XBT_DEBUG("Found a bypass route from '%s' to '%s' with %zu links", src->get_cname(), dst->get_cname(),
                bypassedRoute->links.size());
      return true;
    }
    return false;
  }

  /* Engage recursive search */

  /* (1) find the path to the root routing component */
  std::vector<NetZoneImpl*> path_src;
  NetZoneImpl* current = src->get_englobing_zone();
  while (current != nullptr) {
    path_src.push_back(current);
    current = current->father_;
  }

  std::vector<NetZoneImpl*> path_dst;
  current = dst->get_englobing_zone();
  while (current != nullptr) {
    path_dst.push_back(current);
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
  const BypassRoute* bypassedRoute = nullptr;
  std::pair<kernel::routing::NetPoint*, kernel::routing::NetPoint*> key;
  for (int max = 0; max <= max_index; max++) {
    for (int i = 0; i < max; i++) {
      if (i <= max_index_src && max <= max_index_dst) {
        key = {path_src.at(i)->netpoint_, path_dst.at(max)->netpoint_};
        auto bpr = bypass_routes_.find(key);
        if (bpr != bypass_routes_.end()) {
          bypassedRoute = bpr->second;
          break;
        }
      }
      if (max <= max_index_src && i <= max_index_dst) {
        key = {path_src.at(max)->netpoint_, path_dst.at(i)->netpoint_};
        auto bpr = bypass_routes_.find(key);
        if (bpr != bypass_routes_.end()) {
          bypassedRoute = bpr->second;
          break;
        }
      }
    }

    if (bypassedRoute)
      break;

    if (max <= max_index_src && max <= max_index_dst) {
      key = {path_src.at(max)->netpoint_, path_dst.at(max)->netpoint_};
      auto bpr = bypass_routes_.find(key);
      if (bpr != bypass_routes_.end()) {
        bypassedRoute = bpr->second;
        break;
      }
    }
  }

  /* (4) If we have the bypass, use it. If not, caller will do the Right Thing. */
  if (bypassedRoute) {
    XBT_DEBUG("Found a bypass route from '%s' to '%s' with %zu links. We may have to complete it with recursive "
              "calls to getRoute",
              src->get_cname(), dst->get_cname(), bypassedRoute->links.size());
    if (src != key.first)
      get_global_route(src, bypassedRoute->gw_src, links, latency);
    for (resource::LinkImpl* const& link : bypassedRoute->links) {
      links.push_back(link);
      if (latency)
        *latency += link->get_latency();
    }
    if (dst != key.second)
      get_global_route(bypassedRoute->gw_dst, dst, links, latency);
    return true;
  }
  XBT_DEBUG("No bypass route from '%s' to '%s'.", src->get_cname(), dst->get_cname());
  return false;
}

void NetZoneImpl::get_global_route(NetPoint* src, NetPoint* dst,
                                   /* OUT */ std::vector<resource::LinkImpl*>& links, double* latency)
{
  RouteCreationArgs route;

  XBT_DEBUG("Resolve route from '%s' to '%s'", src->get_cname(), dst->get_cname());

  /* Find how src and dst are interconnected */
  NetZoneImpl *common_ancestor;
  NetZoneImpl *src_ancestor;
  NetZoneImpl *dst_ancestor;
  find_common_ancestors(src, dst, &common_ancestor, &src_ancestor, &dst_ancestor);
  XBT_DEBUG("elements_father: common ancestor '%s' src ancestor '%s' dst ancestor '%s'", common_ancestor->get_cname(),
            src_ancestor->get_cname(), dst_ancestor->get_cname());

  /* Check whether a direct bypass is defined. If so, use it and bail out */
  if (common_ancestor->get_bypass_route(src, dst, links, latency))
    return;

  /* If src and dst are in the same netzone, life is good */
  if (src_ancestor == dst_ancestor) { /* SURF_ROUTING_BASE */
    route.link_list = std::move(links);
    common_ancestor->get_local_route(src, dst, &route, latency);
    links = std::move(route.link_list);
    return;
  }

  /* Not in the same netzone, no bypass. We'll have to find our path between the netzones recursively */

  common_ancestor->get_local_route(src_ancestor->netpoint_, dst_ancestor->netpoint_, &route, latency);
  xbt_assert((route.gw_src != nullptr) && (route.gw_dst != nullptr), "Bad gateways for route from '%s' to '%s'.",
             src->get_cname(), dst->get_cname());

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != route.gw_src)
    get_global_route(src, route.gw_src, links, latency);
  for (auto const& link : route.link_list)
    links.push_back(link);

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  if (route.gw_dst != dst)
    get_global_route(route.gw_dst, dst, links, latency);
}
} // namespace routing
} // namespace kernel
} // namespace simgrid
