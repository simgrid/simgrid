/* Copyright (c) 2009-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/RoutedZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "xbt/dict.h"
#include "xbt/graph.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/asserts.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_routing_generic, ker_platform, "Kernel Generic Routing");

/* ***************************************************************** */
/* *********************** GENERIC METHODS ************************* */

namespace simgrid::kernel::routing {

RoutedZone::RoutedZone(const std::string& name) : NetZoneImpl(name) {}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */
/* change a route containing link names into a route containing link entities */
Route* RoutedZone::new_extended_route(RoutingMode hierarchy, NetPoint* gw_src, NetPoint* gw_dst,
                                      const std::vector<resource::StandardLinkImpl*>& link_list, bool preserve_order)
{
  xbt_enforce(hierarchy != RoutingMode::recursive || (gw_src && gw_dst), "nullptr is obviously a deficient gateway");

  auto* result = new Route();
  if (hierarchy == RoutingMode::recursive) {
    result->gw_src_ = gw_src;
    result->gw_dst_ = gw_dst;
  }

  if (preserve_order)
    result->link_list_ = link_list;
  else
    result->link_list_.assign(link_list.rbegin(), link_list.rend()); // reversed
  result->link_list_.shrink_to_fit();

  return result;
}

void RoutedZone::get_route_check_params(const NetPoint* src, const NetPoint* dst) const
{
  xbt_enforce(src, "Cannot have a route with (nullptr) source");
  xbt_enforce(dst, "Cannot have a route with (nullptr) destination");

  const NetZoneImpl* src_as = src->get_englobing_zone();
  const NetZoneImpl* dst_as = dst->get_englobing_zone();

  xbt_enforce(src_as == dst_as,
             "Internal error: %s@%s and %s@%s are not in the same netzone as expected. Please report that bug.",
             src->get_cname(), src_as->get_cname(), dst->get_cname(), dst_as->get_cname());

  xbt_enforce(this == dst_as,
             "Internal error: route destination %s@%s is not in netzone %s as expected (route source: "
             "%s@%s). Please report that bug.",
             src->get_cname(), dst->get_cname(), src_as->get_cname(), dst_as->get_cname(), get_cname());
}

void RoutedZone::add_route_check_params(NetPoint* src, NetPoint* dst, NetPoint* gw_src, NetPoint* gw_dst,
                                        const std::vector<s4u::LinkInRoute>& link_list, bool symmetrical) const
{
  get_route_check_params(src, dst);
  const char* srcName = src->get_cname();
  const char* dstName = dst->get_cname();

  if (not gw_dst || not gw_src) {
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", srcName, dstName);
    xbt_enforce(not link_list.empty(), "Empty route (between %s and %s) forbidden.", srcName, dstName);
    xbt_enforce(not src->is_netzone(),
               "When defining a route, src cannot be a netzone such as '%s'. Did you meant to have a NetzoneRoute?",
               srcName);
    xbt_enforce(not dst->is_netzone(),
               "When defining a route, dst cannot be a netzone such as '%s'. Did you meant to have a NetzoneRoute?",
               dstName);
    NetZoneImpl::on_route_creation(symmetrical, src, dst, gw_src, gw_dst, get_link_list_impl(link_list, false));
  } else {
    XBT_DEBUG("Load NetzoneRoute from %s@%s to %s@%s", srcName, gw_src->get_cname(), dstName, gw_dst->get_cname());
    xbt_enforce(src->is_netzone(), "When defining a NetzoneRoute, src must be a netzone but '%s' is not", srcName);
    xbt_enforce(dst->is_netzone(), "When defining a NetzoneRoute, dst must be a netzone but '%s' is not", dstName);

    xbt_enforce(gw_src->is_host() || gw_src->is_router(),
               "When defining a NetzoneRoute, gw_src must be a host or a router but '%s' is not.", srcName);
    xbt_enforce(gw_dst->is_host() || gw_dst->is_router(),
               "When defining a NetzoneRoute, gw_dst must be a host or a router but '%s' is not.", dstName);

    xbt_enforce(gw_src != gw_dst, "Cannot define a NetzoneRoute from '%s' to itself", gw_src->get_cname());

    xbt_enforce(src, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->get_cname(), dstName,
               gw_dst->get_cname(), srcName);
    xbt_enforce(dst, "Cannot add a route from %s@%s to %s@%s: %s does not exist.", srcName, gw_src->get_cname(), dstName,
               gw_dst->get_cname(), dstName);
    xbt_enforce(not link_list.empty(), "Empty route (between %s@%s and %s@%s) forbidden.", srcName, gw_src->get_cname(),
               dstName, gw_dst->get_cname());
    const auto* netzone_src = get_netzone_recursive(src);
    xbt_enforce(netzone_src->is_component_recursive(gw_src),
               "Invalid NetzoneRoute from %s@%s to %s@%s: gw_src %s belongs to %s, not to %s.", srcName,
               gw_src->get_cname(), dstName, gw_dst->get_cname(), gw_src->get_cname(),
               gw_src->get_englobing_zone()->get_cname(), srcName);

    const auto* netzone_dst = get_netzone_recursive(dst);
    xbt_enforce(netzone_dst->is_component_recursive(gw_dst),
               "Invalid NetzoneRoute from %s@%s to %s@%s: gw_dst %s belongs to %s, not to %s.", srcName,
               gw_src->get_cname(), dstName, gw_dst->get_cname(), gw_dst->get_cname(),
               gw_dst->get_englobing_zone()->get_cname(), dst->get_cname());
    NetZoneImpl::on_route_creation(symmetrical, gw_src, gw_dst, gw_src, gw_dst, get_link_list_impl(link_list, false));
  }
}
} // namespace simgrid::kernel::routing
