/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/include/simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_route);

namespace simgrid {
namespace kernel {
namespace routing {

/* Pick the right models for CPU, net and host, and call their model_init_preparse */
static void surf_config_models_setup()
{
  std::string host_model_name    = simgrid::config::get_value<std::string>("host/model");
  std::string network_model_name = simgrid::config::get_value<std::string>("network/model");
  std::string cpu_model_name     = simgrid::config::get_value<std::string>("cpu/model");
  std::string disk_model_name    = simgrid::config::get_value<std::string>("disk/model");

  /* The compound host model is needed when using non-default net/cpu models */
  if ((not simgrid::config::is_default("network/model") || not simgrid::config::is_default("cpu/model")) &&
      simgrid::config::is_default("host/model")) {
    host_model_name = "compound";
    simgrid::config::set_value("host/model", host_model_name);
  }

  XBT_DEBUG("host model: %s", host_model_name.c_str());
  if (host_model_name == "compound") {
    xbt_assert(not cpu_model_name.empty(), "Set a cpu model to use with the 'compound' host model");
    xbt_assert(not network_model_name.empty(), "Set a network model to use with the 'compound' host model");

    const auto* cpu_model = find_model_description(surf_cpu_model_description, cpu_model_name);
    cpu_model->model_init_preparse();

    const auto* network_model = find_model_description(surf_network_model_description, network_model_name);
    network_model->model_init_preparse();
  }

  XBT_DEBUG("Call host_model_init");
  const auto* host_model = find_model_description(surf_host_model_description, host_model_name);
  host_model->model_init_preparse();

  XBT_DEBUG("Call vm_model_init");
  /* ideally we should get back the pointer to CpuModel from model_init_preparse(), but this
   * requires changing the declaration of surf_cpu_model_description.
   * To be reviewed in the future */
  surf_vm_model_init_HL13(
      simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->get_cpu_pm_model().get());

  XBT_DEBUG("Call disk_model_init");
  const auto* disk_model = find_model_description(surf_disk_model_description, disk_model_name);
  disk_model->model_init_preparse();
}

NetZoneImpl::NetZoneImpl(const std::string& name) : piface_(this), name_(name)
{
  /* workaroud: first netzoneImpl will be the root netzone.
   * Without globals and with current surf_*_model_description init functions, we need
   * the root netzone to exist when creating the models.
   * This was usually done at sg_platf.cpp, during XML parsing */
  if (not s4u::Engine::get_instance()->get_netzone_root()) {
    s4u::Engine::get_instance()->set_netzone_root(&piface_);
    /* root netzone set, initialize models */
    simgrid::s4u::Engine::on_platform_creation();

    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|zone|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer come down to zone creations, so putting this verification here is correct.
     */
    surf_config_models_setup();
  }

  xbt_assert(nullptr == s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()),
             "Refusing to create a second NetZone called '%s'.", get_cname());
  netpoint_ = new NetPoint(name_, NetPoint::Type::NetZone);
  XBT_DEBUG("NetZone '%s' created with the id '%u'", get_cname(), netpoint_->id());
  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */
  simgrid::s4u::NetZone::on_creation(piface_); // notify the signal
}

NetZoneImpl::~NetZoneImpl()
{
  for (auto const& nz : children_)
    delete nz;

  for (auto const& kv : bypass_routes_)
    delete kv.second;

  s4u::Engine::get_instance()->netpoint_unregister(netpoint_);
}

void NetZoneImpl::add_child(NetZoneImpl* new_zone)
{
  xbt_assert(not sealed_, "Cannot add a new child to the sealed zone %s", get_cname());
  /* set the father behavior */
  hierarchy_ = RoutingMode::recursive;
  children_.push_back(new_zone);
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

s4u::Host* NetZoneImpl::create_host(const std::string& name, const std::vector<double>& speed_per_pstate)
{
  auto* res = (new surf::HostImpl(name))->get_iface();
  res->set_netpoint((new NetPoint(name, NetPoint::Type::Host))->set_englobing_zone(this));

  cpu_model_pm_->create_cpu(res, speed_per_pstate);

  return res;
}

s4u::Link* NetZoneImpl::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  return network_model_->create_link(name, bandwidths)->get_iface();
}

s4u::Disk* NetZoneImpl::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  auto* l = disk_model_->create_disk(name, read_bandwidth, write_bandwidth);

  return l->get_iface();
}

NetPoint* NetZoneImpl::create_router(const std::string& name)
{
  xbt_assert(nullptr == s4u::Engine::get_instance()->netpoint_by_name_or_null(name),
             "Refusing to create a router named '%s': this name already describes a node.", name.c_str());

  return (new NetPoint(name, NetPoint::Type::Router))->set_englobing_zone(this);
}
int NetZoneImpl::add_component(NetPoint* elm)
{
  vertices_.push_back(elm);
  return vertices_.size() - 1; // The rank of the newly created object
}

void NetZoneImpl::add_route(NetPoint* /*src*/, NetPoint* /*dst*/, NetPoint* /*gw_src*/, NetPoint* /*gw_dst*/,
                            const std::vector<resource::LinkImpl*>& /*link_list_*/, bool /*symmetrical*/)
{
  xbt_die("NetZone '%s' does not accept new routes (wrong class).", get_cname());
}

void NetZoneImpl::add_bypass_route(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                   std::vector<resource::LinkImpl*>& link_list_, bool /* symmetrical */)
{
  /* Argument validity checks */
  if (gw_dst) {
    XBT_DEBUG("Load bypassNetzoneRoute from %s@%s to %s@%s", src->get_cname(), gw_src->get_cname(), dst->get_cname(),
              gw_dst->get_cname());
    xbt_assert(not link_list_.empty(), "Bypass route between %s@%s and %s@%s cannot be empty.", src->get_cname(),
               gw_src->get_cname(), dst->get_cname(), gw_dst->get_cname());
    xbt_assert(bypass_routes_.find({src, dst}) == bypass_routes_.end(),
               "The bypass route between %s@%s and %s@%s already exists.", src->get_cname(), gw_src->get_cname(),
               dst->get_cname(), gw_dst->get_cname());
  } else {
    XBT_DEBUG("Load bypassRoute from %s to %s", src->get_cname(), dst->get_cname());
    xbt_assert(not link_list_.empty(), "Bypass route between %s and %s cannot be empty.", src->get_cname(),
               dst->get_cname());
    xbt_assert(bypass_routes_.find({src, dst}) == bypass_routes_.end(),
               "The bypass route between %s and %s already exists.", src->get_cname(), dst->get_cname());
  }

  /* Build a copy that will be stored in the dict */
  auto* newRoute = new BypassRoute(gw_src, gw_dst);
  for (auto const& link : link_list_)
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
    current = current->get_parent();
  }
  std::vector<NetZoneImpl*> path_dst;
  current = dst->get_englobing_zone();
  while (current != nullptr) {
    path_dst.push_back(current);
    current = current->get_parent();
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
    current = current->parent_;
  }

  std::vector<NetZoneImpl*> path_dst;
  current = dst->get_englobing_zone();
  while (current != nullptr) {
    path_dst.push_back(current);
    current = current->parent_;
  }

  /* (2) find the common father */
  while (path_src.size() > 1 && path_dst.size() > 1 &&
         path_src.at(path_src.size() - 1) == path_dst.at(path_dst.size() - 1)) {
    path_src.pop_back();
    path_dst.pop_back();
  }

  /* (3) Search for a bypass making the path up to the ancestor useless */
  const BypassRoute* bypassedRoute = nullptr;
  std::pair<kernel::routing::NetPoint*, kernel::routing::NetPoint*> key;
  // Search for a bypass with the given indices. Returns true if found. Initialize variables `bypassedRoute' and `key'.
  auto lookup = [&bypassedRoute, &key, &path_src, &path_dst, this](unsigned src_index, unsigned dst_index) {
    if (src_index < path_src.size() && dst_index < path_dst.size()) {
      key      = {path_src[src_index]->netpoint_, path_dst[dst_index]->netpoint_};
      auto bpr = bypass_routes_.find(key);
      if (bpr != bypass_routes_.end()) {
        bypassedRoute = bpr->second;
        return true;
      }
    }
    return false;
  };

  for (unsigned max = 0, max_index = std::max(path_src.size(), path_dst.size()); max < max_index; max++) {
    for (unsigned i = 0; i < max; i++) {
      if (lookup(i, max) || lookup(max, i))
        break;
    }
    if (bypassedRoute || lookup(max, max))
      break;
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
  Route route;

  XBT_DEBUG("Resolve route from '%s' to '%s'", src->get_cname(), dst->get_cname());

  /* Find how src and dst are interconnected */
  NetZoneImpl* common_ancestor;
  NetZoneImpl* src_ancestor;
  NetZoneImpl* dst_ancestor;
  find_common_ancestors(src, dst, &common_ancestor, &src_ancestor, &dst_ancestor);
  XBT_DEBUG("elements_father: common ancestor '%s' src ancestor '%s' dst ancestor '%s'", common_ancestor->get_cname(),
            src_ancestor->get_cname(), dst_ancestor->get_cname());

  /* Check whether a direct bypass is defined. If so, use it and bail out */
  if (common_ancestor->get_bypass_route(src, dst, links, latency))
    return;

  /* If src and dst are in the same netzone, life is good */
  if (src_ancestor == dst_ancestor) { /* SURF_ROUTING_BASE */
    route.link_list_ = std::move(links);
    common_ancestor->get_local_route(src, dst, &route, latency);
    links = std::move(route.link_list_);
    return;
  }

  /* Not in the same netzone, no bypass. We'll have to find our path between the netzones recursively */

  common_ancestor->get_local_route(src_ancestor->netpoint_, dst_ancestor->netpoint_, &route, latency);
  xbt_assert((route.gw_src_ != nullptr) && (route.gw_dst_ != nullptr), "Bad gateways for route from '%s' to '%s'.",
             src->get_cname(), dst->get_cname());

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != route.gw_src_)
    get_global_route(src, route.gw_src_, links, latency);
  links.insert(links.end(), begin(route.link_list_), end(route.link_list_));

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  if (route.gw_dst_ != dst)
    get_global_route(route.gw_dst_, dst, links, latency);
}

void NetZoneImpl::seal()
{
  /* already sealed netzone */
  if (sealed_)
    return;
  do_seal(); // derived class' specific sealing procedure

  /* seals sub-netzones and hosts */
  for (auto* host : get_all_hosts()) {
    host->seal();
  }
  for (auto* sub_net : get_children()) {
    sub_net->seal();
  }
  sealed_ = true;
  s4u::NetZone::on_seal(piface_);
}

void NetZoneImpl::set_parent(NetZoneImpl* parent)
{
  xbt_assert(not sealed_, "Impossible to set parent to an already sealed NetZone(%s)", this->get_cname());
  parent_ = parent;
  netpoint_->set_englobing_zone(parent_);
  if (parent) {
    /* adding this class as child */
    parent->add_child(this);
    /* copying models from parent host, to be reviewed when we allow multi-models */
    set_network_model(parent->get_network_model());
    set_cpu_pm_model(parent->get_cpu_pm_model());
    set_cpu_vm_model(parent->get_cpu_vm_model());
    set_disk_model(parent->get_disk_model());
    set_host_model(parent->get_host_model());
  }
}

void NetZoneImpl::set_network_model(std::shared_ptr<resource::NetworkModel> netmodel)
{
  xbt_assert(not sealed_, "Impossible to set network model to an already sealed NetZone(%s)", this->get_cname());
  network_model_ = std::move(netmodel);
}

void NetZoneImpl::set_cpu_vm_model(std::shared_ptr<resource::CpuModel> cpu_model)
{
  xbt_assert(not sealed_, "Impossible to set CPU model to an already sealed NetZone(%s)", this->get_cname());
  cpu_model_vm_ = std::move(cpu_model);
}

void NetZoneImpl::set_cpu_pm_model(std::shared_ptr<resource::CpuModel> cpu_model)
{
  xbt_assert(not sealed_, "Impossible to set CPU model to an already sealed NetZone(%s)", this->get_cname());
  cpu_model_pm_ = std::move(cpu_model);
}

void NetZoneImpl::set_disk_model(std::shared_ptr<resource::DiskModel> disk_model)
{
  xbt_assert(not sealed_, "Impossible to set disk model to an already sealed NetZone(%s)", this->get_cname());
  disk_model_ = std::move(disk_model);
}

void NetZoneImpl::set_host_model(std::shared_ptr<surf::HostModel> host_model)
{
  xbt_assert(not sealed_, "Impossible to set host model to an already sealed NetZone(%s)", this->get_cname());
  host_model_ = std::move(host_model);
}

} // namespace routing
} // namespace kernel
} // namespace simgrid
