/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_routing_private.hpp"
#include "src/surf/surf_routing_full.hpp"
#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i,j) p_routingTable[(i)+(j)*table_size]

namespace simgrid {
namespace surf {
  AsFull::AsFull(const char*name)
    : AsGeneric(name)
  {
  }

void AsFull::Seal() {
  int i;
  sg_platf_route_cbarg_t e_route;

  /* set utils vars */
  int table_size = (int)xbt_dynar_length(vertices_);

  /* Create table if necessary */
  if (!p_routingTable)
    p_routingTable = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Add the loopback if needed */
  if (routing_platf->p_loopback && hierarchy_ == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
        xbt_dynar_push(e_route->link_list, &routing_platf->p_loopback);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

AsFull::~AsFull(){
  if (p_routingTable) {
    int table_size = (int)xbt_dynar_length(vertices_);
    int i, j;
    /* Delete routing table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++) {
        if (TO_ROUTE_FULL(i,j)){
          xbt_dynar_free(&TO_ROUTE_FULL(i,j)->link_list);
          xbt_free(TO_ROUTE_FULL(i,j));
        }
      }
    xbt_free(p_routingTable);
  }
}

xbt_dynar_t AsFull::getOneLinkRoutes()
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(Onelink*), xbt_free_f);

  int src, dst;
  int table_size = xbt_dynar_length(vertices_);

  for(src=0; src < table_size; src++) {
    for(dst=0; dst< table_size; dst++) {
      sg_platf_route_cbarg_t route = TO_ROUTE_FULL(src,dst);
      if (route) {
        if (xbt_dynar_length(route->link_list) == 1) {
          void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
          Onelink *onelink;
          if (hierarchy_ == SURF_ROUTING_BASE) {
          NetCard *tmp_src = xbt_dynar_get_as(vertices_, src, sg_netcard_t);
            tmp_src->setId(src);
          NetCard *tmp_dst = xbt_dynar_get_as(vertices_, dst, sg_netcard_t);
          tmp_dst->setId(dst);
            onelink = new Onelink(link, tmp_src, tmp_dst);
          } else if (hierarchy_ == SURF_ROUTING_RECURSIVE)
            onelink = new Onelink(link, route->gw_src, route->gw_dst);
          else
            onelink = new Onelink(link, NULL, NULL);
          xbt_dynar_push(ret, &onelink);
          XBT_DEBUG("Push route from '%d' to '%d'",
              src,
              dst);
        }
      }
    }
  }
  return ret;
}

void AsFull::getRouteAndLatency(NetCard *src, NetCard *dst, sg_platf_route_cbarg_t res, double *lat)
{
  XBT_DEBUG("full_get_route_and_latency from %s[%d] to %s[%d]",
      src->name(),
      src->id(),
      dst->name(),
      dst->id());

  /* set utils vars */
  size_t table_size = xbt_dynar_length(vertices_);

  sg_platf_route_cbarg_t e_route = NULL;
  void *link;
  unsigned int cpt = 0;

  e_route = TO_ROUTE_FULL(src->id(), dst->id());

  if (e_route) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    xbt_dynar_foreach(e_route->link_list, cpt, link) {
      xbt_dynar_push(res->link_list, &link);
      if (lat)
        *lat += static_cast<Link*>(link)->getLatency();
    }
  }
}

static int full_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

void AsFull::parseRoute(sg_platf_route_cbarg_t route)
{
  int as_route = 0;

  const char *src = route->src;
  const char *dst = route->dst;
  NetCard *src_net_elm = sg_netcard_by_name_or_null(src);
  NetCard *dst_net_elm = sg_netcard_by_name_or_null(dst);

  xbt_assert(src_net_elm, "Network elements %s not found", src);
  xbt_assert(dst_net_elm, "Network elements %s not found", dst);

  size_t table_size = xbt_dynar_length(vertices_);

  xbt_assert(!xbt_dynar_is_empty(route->link_list),
      "Invalid count of links, must be greater than zero (%s,%s)",
      src, dst);

  if (!p_routingTable)
    p_routingTable = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  if (TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id())) {
    char *link_name;
    unsigned int i;
    xbt_dynar_t link_route_to_test =
        xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
    xbt_dynar_foreach(route->link_list, i, link_name) {
      void *link = Link::byName(link_name);
      xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
      xbt_dynar_push(link_route_to_test, &link);
    }
    if (xbt_dynar_compare(TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id())->link_list,
        link_route_to_test, full_pointer_resource_cmp)) {
      surf_parse_error("A route between \"%s\" and \"%s\" already exists "
          "with a different content. "
          "If you are trying to define a reverse route, "
          "you must set the symmetrical=no attribute to "
          "your routes tags.", src, dst);
    } else {
      surf_parse_warn("Ignoring the identical redefinition of the route "
          "between \"%s\" and \"%s\"", src, dst);
    }
  } else {
    if (!route->gw_src && !route->gw_dst)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
    else {
      // FIXME We can call a gw which is down the current AS (cf g5k.xml) but not upper.
      //      AS_t subas = xbt_dict_get_or_null(rc->routing_sons, src);
      //      if (subas == NULL)
      //        surf_parse_error("The source of an ASroute must be a sub-AS "
      //                         "declared within the current AS, "
      //                         "but '%s' is not an AS within '%s'", src, rc->name);
      //      if (subas->to_index
      //          && xbt_dict_get_or_null(subas->to_index, route->src_gateway) == NULL)
      //        surf_parse_error("In an ASroute, source gateway must be part of "
      //                         "the source sub-AS (in particular, being in a "
      //                         "sub-sub-AS is not allowed), "
      //                         "but '%s' is not in '%s'.",
      //                         route->src_gateway, subas->name);
      //
      //      subas = xbt_dict_get_or_null(rc->routing_sons, dst);
      //      if (subas == NULL)
      //        surf_parse_error("The destination of an ASroute must be a sub-AS "
      //                         "declared within the current AS, "
      //                         "but '%s' is not an AS within '%s'", dst, rc->name);
      //      if (subas->to_index
      //          && xbt_dict_get_or_null(subas->to_index, route->dst_gateway) == NULL)
      //        surf_parse_error("In an ASroute, destination gateway must be "
      //                         "part of the destination sub-AS (in particular, "
      //                         "in a sub-sub-AS is not allowed), "
      //                         "but '%s' is not in '%s'.",
      //                         route->dst_gateway, subas->name);
      as_route = 1;
      XBT_DEBUG("Load ASroute from \"%s\" to \"%s\"", src, dst);
      if (!route->gw_src ||
          route->gw_src->getRcType() == SURF_NETWORK_ELEMENT_NULL)
      surf_parse_error("The src_gateway \"%s\" does not exist!",
                route->gw_src ? route->gw_src->name() : "(null)");
      if (!route->gw_dst ||
          route->gw_dst->getRcType() == SURF_NETWORK_ELEMENT_NULL)
      surf_parse_error("The dst_gateway \"%s\" does not exist!",
                route->gw_dst ? route->gw_dst->name() : "(null)");
      XBT_DEBUG("ASroute goes from \"%s\" to \"%s\"",
                route->gw_src->name(), route->gw_dst->name());
    }
    TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id()) = newExtendedRoute(hierarchy_, route, 1);
    xbt_dynar_shrink(TO_ROUTE_FULL(src_net_elm->id(), dst_net_elm->id())->link_list, 0);
  }

  if ( (route->symmetrical == TRUE && as_route == 0)
      || (route->symmetrical == TRUE && as_route == 1)
  ) {
    if (route->gw_dst && route->gw_src) {
      sg_netcard_t gw_tmp;
      gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }
    if (TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id())) {
      char *link_name;
      unsigned int i;
      xbt_dynar_t link_route_to_test =
          xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
      for (i = xbt_dynar_length(route->link_list); i > 0; i--) {
        link_name = xbt_dynar_get_as(route->link_list, i - 1, char *);
        void *link = Link::byName(link_name);
        xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
        xbt_dynar_push(link_route_to_test, &link);
      }
      xbt_assert(!xbt_dynar_compare(TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id())->link_list,
          link_route_to_test,
          full_pointer_resource_cmp),
          "The route between \"%s\" and \"%s\" already exists", src,
          dst);
    } else {
      if (!route->gw_dst && !route->gw_src)
        XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
            dst, route->gw_src->name(), src, route->gw_dst->name());
      TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id()) = newExtendedRoute(hierarchy_, route, 0);
      xbt_dynar_shrink(TO_ROUTE_FULL(dst_net_elm->id(), src_net_elm->id())->link_list, 0);
    }
  }
  xbt_dynar_free(&route->link_list);
}

}
}
