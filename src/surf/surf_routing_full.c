/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_global_t global_routing;
extern int surf_parse_lineno;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");

#define TO_ROUTE_FULL(i,j) routing->routing_table[(i)+(j)*table_size]

/* Routing model structure */

typedef struct s_routing_component_full {
  s_as_t generic_routing;
  route_t *routing_table;
} s_routing_component_full_t, *routing_component_full_t;

/* Business methods */
static xbt_dynar_t full_get_onelink_routes(AS_t rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
  xbt_dict_cursor_t c1 = NULL, c2 = NULL;
  char *k1, *d1, *k2, *d2;
  xbt_dict_foreach(routing->generic_routing.to_index, c1, k1, d1) {
    xbt_dict_foreach(routing->generic_routing.to_index, c2, k2, d2) {
      int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, k1);
      int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, k2);
      xbt_assert(src_id && dst_id,
                 "Ask for route \"from\"(%s)  or \"to\"(%s) "
                 "no found in the local table", k1, k2);
      route_t route = TO_ROUTE_FULL(*src_id, *dst_id);
      if (route) {
        if (xbt_dynar_length(route->link_list) == 1) {
          void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (routing->generic_routing.hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_strdup(k1);
            onelink->dst = xbt_strdup(k2);
          } else if (routing->generic_routing.hierarchy ==
                     SURF_ROUTING_RECURSIVE) {
            onelink->src = xbt_strdup(route->src_gateway);
            onelink->dst = xbt_strdup(route->dst_gateway);
          }
          xbt_dynar_push(ret, &onelink);
        }
      }
    }
  }
  return ret;
}

static void full_get_route_and_latency(AS_t rc,
                                       const char *src, const char *dst,
                                       route_t res, double *lat)
{

  /* set utils vars */
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  int *src_id = xbt_dict_get_or_null(routing->generic_routing.to_index, src);
  int *dst_id = xbt_dict_get_or_null(routing->generic_routing.to_index, dst);

  if (!src_id || !dst_id)
    THROWF(arg_error, 0, "No route from '%s' to '%s'", src, dst);

  route_t e_route = NULL;
  void *link;
  unsigned int cpt = 0;

  e_route = TO_ROUTE_FULL(*src_id, *dst_id);

  if (e_route) {
    res->src_gateway = xbt_strdup(e_route->src_gateway);
    res->dst_gateway = xbt_strdup(e_route->dst_gateway);
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
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);
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
  route_t e_route;

  /* set utils vars */
  routing_component_full_t routing =
      ((routing_component_full_t) current_routing);
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  /* Create table if necessary */
  if (!routing->routing_table)
    routing->routing_table = xbt_new0(route_t, table_size * table_size);

  /* Add the loopback if needed */
  if (current_routing->hierarchy == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      e_route = TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_route_t, 1);
        e_route->src_gateway = NULL;
        e_route->dst_gateway = NULL;
        e_route->link_list = xbt_dynar_new(global_routing->size_of_link, NULL);
        xbt_dynar_push(e_route->link_list, &global_routing->loopback);
        TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

static int full_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

void model_full_set_route(AS_t rc, const char *src,
                          const char *dst, route_t route)
{
  int *src_id, *dst_id;
  src_id = xbt_dict_get_or_null(rc->to_index, src);
  dst_id = xbt_dict_get_or_null(rc->to_index, dst);
  routing_component_full_t routing = (routing_component_full_t) rc;
  size_t table_size = xbt_dict_length(routing->generic_routing.to_index);

  xbt_assert(src_id, "Network elements %s not found", src);
  xbt_assert(dst_id, "Network elements %s not found", dst);

  xbt_assert(!xbt_dynar_is_empty(route->link_list),
             "Invalid count of links, must be greater than zero (%s,%s)",
             src, dst);

  if (!routing->routing_table)
    routing->routing_table = xbt_new0(route_t, table_size * table_size);

  if (TO_ROUTE_FULL(*src_id, *dst_id)) {
    char *link_name;
    unsigned int i;
    xbt_dynar_t link_route_to_test =
        xbt_dynar_new(global_routing->size_of_link, NULL);
    xbt_dynar_foreach(route->link_list, i, link_name) {
      void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
      xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
      xbt_dynar_push(link_route_to_test, &link);
    }
    if (xbt_dynar_compare(TO_ROUTE_FULL(*src_id, *dst_id)->link_list,
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
    if (!route->dst_gateway && !route->src_gateway)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
    else {
// FIXME We can call a gw wich is down the current AS (cf g5k.xml) but not upper.
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

      XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
                src, route->src_gateway, dst, route->dst_gateway);
      if (routing_get_network_element_type(route->dst_gateway) ==
          SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The dst_gateway '%s' does not exist!", route->dst_gateway);
      if (routing_get_network_element_type(route->src_gateway) ==
          SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The src_gateway '%s' does not exist!", route->src_gateway);
    }
    TO_ROUTE_FULL(*src_id, *dst_id) =
        generic_new_extended_route(rc->hierarchy, route, 1);
    xbt_dynar_shrink(TO_ROUTE_FULL(*src_id, *dst_id)->link_list, 0);
  }

  if (A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES
      || A_surfxml_ASroute_symmetrical == A_surfxml_ASroute_symmetrical_YES) {
    if (route->dst_gateway && route->src_gateway) {
      char *gw_tmp;
      gw_tmp = route->src_gateway;
      route->src_gateway = route->dst_gateway;
      route->dst_gateway = gw_tmp;
    }
    if (TO_ROUTE_FULL(*dst_id, *src_id)) {
      char *link_name;
      unsigned int i;
      xbt_dynar_t link_route_to_test =
          xbt_dynar_new(global_routing->size_of_link, NULL);
      for (i = xbt_dynar_length(route->link_list); i > 0; i--) {
        link_name = xbt_dynar_get_as(route->link_list, i - 1, void *);
        void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
        xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
        xbt_dynar_push(link_route_to_test, &link);
      }
      xbt_assert(!xbt_dynar_compare(TO_ROUTE_FULL(*dst_id, *src_id)->link_list,
                                    link_route_to_test,
                                    full_pointer_resource_cmp),
                 "The route between \"%s\" and \"%s\" already exists", src,
                 dst);
    } else {
      if (!route->dst_gateway && !route->src_gateway)
        XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
                  dst, route->src_gateway, src, route->dst_gateway);
      TO_ROUTE_FULL(*dst_id, *src_id) =
          generic_new_extended_route(rc->hierarchy, route, 0);
      xbt_dynar_shrink(TO_ROUTE_FULL(*dst_id, *src_id)->link_list, 0);
    }
  }
}
