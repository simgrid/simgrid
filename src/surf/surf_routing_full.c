/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_platf_t routing_platf;
extern int surf_parse_lineno;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i,j) routing->routing_table[(i)+(j)*table_size]

/* Routing model structure */

typedef struct s_routing_component_full {
  s_as_t generic_routing;
  sg_platf_route_cbarg_t *routing_table;
} s_routing_component_full_t, *routing_component_full_t;

/* Business methods */
static xbt_dynar_t full_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);
  routing_component_full_t routing = (routing_component_full_t) rc;

  int src,dst;
  int table_size = xbt_dynar_length(rc->index_network_elm);

  for(src=0; src < table_size; src++) {
    for(dst=0; dst< table_size; dst++) {
      sg_platf_route_cbarg_t route = TO_ROUTE_FULL(src, dst);
      if (route) {
        if (xbt_dynar_length(route->link_list) == 1) {
          void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (rc->hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_dynar_get_as(rc->index_network_elm,src,sg_routing_edge_t);
            onelink->src->id = src;
            onelink->dst = xbt_dynar_get_as(rc->index_network_elm,dst,sg_routing_edge_t);
            onelink->dst->id = dst;
          } else if (rc->hierarchy == SURF_ROUTING_RECURSIVE) {
            onelink->src = route->gw_src;
            onelink->dst = route->gw_dst;
          }
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

static void full_get_route_and_latency(AS_t rc,
    sg_routing_edge_t src, sg_routing_edge_t dst,
    sg_platf_route_cbarg_t res, double *lat)
{
  XBT_DEBUG("full_get_route_and_latency from %s[%d] to %s[%d]",
      src->name,
      src->id,
      dst->name,
      dst->id  );

  /* set utils vars */
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dynar_length(routing->generic_routing.index_network_elm);

  sg_platf_route_cbarg_t e_route = NULL;
  void *link;
  unsigned int cpt = 0;

  e_route = TO_ROUTE_FULL(src->id, dst->id);

  if (e_route) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    xbt_dynar_foreach(e_route->link_list, cpt, link) {
      xbt_dynar_push(res->link_list, &link);
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(link);
    }
  }
}

static void full_finalize(AS_t rc)
{
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dynar_length(routing->generic_routing.index_network_elm);
  int i, j;
  if (routing) {
    /* Delete routing table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++)
        generic_free_route(TO_ROUTE_FULL(i, j));
    xbt_free(routing->routing_table);
    model_generic_finalize(rc);
  }
}

/* Creation routing model functions */

AS_t model_full_create(void)
{
  routing_component_full_t new_component = (routing_component_full_t)
          model_generic_create_sized(sizeof(s_routing_component_full_t));

  new_component->generic_routing.parse_route = model_full_set_route;
  new_component->generic_routing.parse_ASroute = model_full_set_route;
  new_component->generic_routing.get_route_and_latency =
      full_get_route_and_latency;
  new_component->generic_routing.get_onelink_routes = full_get_onelink_routes;
  new_component->generic_routing.finalize = full_finalize;

  return (AS_t) new_component;
}

void model_full_end(AS_t current_routing)
{
  unsigned int i;
  sg_platf_route_cbarg_t e_route;

  /* set utils vars */
  routing_component_full_t routing =
      ((routing_component_full_t) current_routing);
  size_t table_size = xbt_dynar_length(routing->generic_routing.index_network_elm);

  /* Create table if necessary */
  if (!routing->routing_table)
    routing->routing_table = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Add the loopback if needed */
  if (routing_platf->loopback && current_routing->hierarchy == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
        xbt_dynar_push(e_route->link_list, &routing_platf->loopback);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

static int full_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

void model_full_set_route(AS_t rc, sg_platf_route_cbarg_t route)
{
  int as_route = 0;
  char *src = (char*)(route->src);
  char *dst = (char*)(route->dst);
  sg_routing_edge_t src_net_elm, dst_net_elm;
  src_net_elm = sg_routing_edge_by_name_or_null(src);
  dst_net_elm = sg_routing_edge_by_name_or_null(dst);

  xbt_assert(src_net_elm, "Network elements %s not found", src);
  xbt_assert(dst_net_elm, "Network elements %s not found", dst);

  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dynar_length(routing->generic_routing.index_network_elm);

  xbt_assert(!xbt_dynar_is_empty(route->link_list),
      "Invalid count of links, must be greater than zero (%s,%s)",
      src, dst);

  if (!routing->routing_table)
    routing->routing_table = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  if (TO_ROUTE_FULL(src_net_elm->id, dst_net_elm->id)) {
    char *link_name;
    unsigned int i;
    xbt_dynar_t link_route_to_test =
        xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
    xbt_dynar_foreach(route->link_list, i, link_name) {
      void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
      xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
      xbt_dynar_push(link_route_to_test, &link);
    }
    if (xbt_dynar_compare(TO_ROUTE_FULL(src_net_elm->id, dst_net_elm->id)->link_list,
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
    if (!route->gw_dst && !route->gw_dst)
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
      XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
          src, route->gw_src->name, dst, route->gw_dst->name);
      if (route->gw_dst->rc_type == SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The dst_gateway '%s' does not exist!", route->gw_dst->name);
      if (route->gw_src->rc_type == SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The src_gateway '%s' does not exist!", route->gw_src->name);
    }
    TO_ROUTE_FULL(src_net_elm->id, dst_net_elm->id) =
        generic_new_extended_route(rc->hierarchy, route, 1);
    xbt_dynar_shrink(TO_ROUTE_FULL(src_net_elm->id, dst_net_elm->id)->link_list, 0);
  }

  if ( (A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES && as_route == 0)
      || (A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES && as_route == 1)
  ) {
    if (route->gw_dst && route->gw_src) {
      sg_routing_edge_t gw_tmp;
      gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }
    if (TO_ROUTE_FULL(dst_net_elm->id, src_net_elm->id)) {
      char *link_name;
      unsigned int i;
      xbt_dynar_t link_route_to_test =
          xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
      for (i = xbt_dynar_length(route->link_list); i > 0; i--) {
        link_name = xbt_dynar_get_as(route->link_list, i - 1, void *);
        void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
        xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
        xbt_dynar_push(link_route_to_test, &link);
      }
      xbt_assert(!xbt_dynar_compare(TO_ROUTE_FULL(dst_net_elm->id, src_net_elm->id)->link_list,
          link_route_to_test,
          full_pointer_resource_cmp),
          "The route between \"%s\" and \"%s\" already exists", src,
          dst);
    } else {
      if (!route->gw_dst && !route->gw_src)
        XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
            dst, route->gw_src->name, src, route->gw_dst->name);
      TO_ROUTE_FULL(dst_net_elm->id, src_net_elm->id) =
          generic_new_extended_route(rc->hierarchy, route, 0);
      xbt_dynar_shrink(TO_ROUTE_FULL(dst_net_elm->id, src_net_elm->id)->link_list, 0);
    }
  }
  xbt_dynar_free(&route->link_list);
}
