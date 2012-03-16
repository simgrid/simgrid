/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pcre.h>               /* regular expression library */

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_private.h"
#include "surf/surf_routing.h"
#include "surf/surfxml_parse_values.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");

static int no_bypassroute_declared = 1;

AS_t model_generic_create_sized(size_t childsize) {
  AS_t new_component = model_none_create_sized(childsize);

  new_component->parse_PU = generic_parse_PU;
  new_component->parse_AS = generic_parse_AS;
  new_component->parse_route = NULL;
  new_component->parse_ASroute = NULL;
  new_component->parse_bypassroute = generic_parse_bypassroute;
  new_component->get_route_and_latency = NULL;
  new_component->get_onelink_routes = NULL;
  new_component->get_bypass_route =
      generic_get_bypassroute;
  new_component->finalize = model_generic_finalize;

  new_component->nb_index = 0;
  new_component->bypassRoutes = xbt_dict_new_homogeneous((void (*)(void *)) generic_free_route);

  return new_component;
}
void model_generic_finalize(AS_t as) {
  xbt_dict_free(&as->bypassRoutes);
  model_none_finalize(as);
}

int generic_parse_PU(AS_t as, const char *name)
{
  XBT_DEBUG("Load process unit \"%s\"", name);
  int id = as->nb_index;
  (as->nb_index)++;
  return id;
}

int generic_parse_AS(AS_t as, const char *name)
{
  XBT_DEBUG("Load Autonomous system \"%s\"", name);
  int id = as->nb_index;
  as->nb_index++;
  return id;
}

void generic_parse_bypassroute(AS_t rc,
                             const char *src, const char *dst,
                             route_t e_route)
{
  XBT_DEBUG("Load bypassRoute from \"%s\" to \"%s\"", src, dst);
  xbt_dict_t dict_bypassRoutes = rc->bypassRoutes;
  char *route_name;

  route_name = bprintf("%s#%s", src, dst);
  xbt_assert(!xbt_dynar_is_empty(e_route->link_list),
             "Invalid count of links, must be greater than zero (%s,%s)",
             src, dst);
  xbt_assert(!xbt_dict_get_or_null(dict_bypassRoutes, route_name),
             "The bypass route between \"%s\"(\"%s\") and \"%s\"(\"%s\") already exists",
             src, e_route->src_gateway->name, dst, e_route->dst_gateway->name);

  route_t new_e_route =
      generic_new_extended_route(SURF_ROUTING_RECURSIVE, e_route, 0);
  xbt_dynar_free(&(e_route->link_list));
  xbt_free(e_route);

  xbt_dict_set(dict_bypassRoutes, route_name, new_e_route, NULL);
  no_bypassroute_declared = 0;
  xbt_free(route_name);
}

/* ************************************************************************** */
/* *********************** GENERIC BUSINESS METHODS ************************* */

xbt_dynar_t generic_get_onelink_routes(AS_t rc) { // FIXME: kill that stub
  xbt_die("\"generic_get_onelink_routes\" not implemented yet");
  return NULL;
}

route_t generic_get_bypassroute(AS_t rc, network_element_t src, network_element_t dst)
{
  // If never set a bypass route return NULL without any further computations
  if(no_bypassroute_declared)
    return NULL;

  xbt_dict_t dict_bypassRoutes = rc->bypassRoutes;
  AS_t src_as, dst_as;
  int index_src, index_dst;
  xbt_dynar_t path_src = NULL;
  xbt_dynar_t path_dst = NULL;
  AS_t current = NULL;
  AS_t *current_src = NULL;
  AS_t *current_dst = NULL;

  char* src_name = src->name;
  char* dst_name = dst->name;

  if (src == NULL || dst == NULL)
    xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
            src_name, dst_name, rc->name);

  src_as = src->rc_component;
  dst_as = dst->rc_component;

  /* (2) find the path to the root routing component */
  path_src = xbt_dynar_new(sizeof(AS_t), NULL);
  current = src_as;
  while (current != NULL) {
    xbt_dynar_push(path_src, &current);
    current = current->routing_father;
  }
  path_dst = xbt_dynar_new(sizeof(AS_t), NULL);
  current = dst_as;
  while (current != NULL) {
    xbt_dynar_push(path_dst, &current);
    current = current->routing_father;
  }

  /* (3) find the common father */
  index_src = path_src->used - 1;
  index_dst = path_dst->used - 1;
  current_src = xbt_dynar_get_ptr(path_src, index_src);
  current_dst = xbt_dynar_get_ptr(path_dst, index_dst);
  while (index_src >= 0 && index_dst >= 0 && *current_src == *current_dst) {
    xbt_dynar_pop_ptr(path_src);
    xbt_dynar_pop_ptr(path_dst);
    index_src--;
    index_dst--;
    current_src = xbt_dynar_get_ptr(path_src, index_src);
    current_dst = xbt_dynar_get_ptr(path_dst, index_dst);
  }

  int max_index_src = path_src->used - 1;
  int max_index_dst = path_dst->used - 1;

  int max_index = max(max_index_src, max_index_dst);
  int i, max;

  route_t e_route_bypass = NULL;

  for (max = 0; max <= max_index; max++) {
    for (i = 0; i < max; i++) {
      if (i <= max_index_src && max <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
                                   (*(AS_t *)
                                    (xbt_dynar_get_ptr(path_src, i)))->name,
                                   (*(AS_t *)
                                    (xbt_dynar_get_ptr(path_dst, max)))->name);
        e_route_bypass = xbt_dict_get_or_null(dict_bypassRoutes, route_name);
        xbt_free(route_name);
      }
      if (e_route_bypass)
        break;
      if (max <= max_index_src && i <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
                                   (*(AS_t *)
                                    (xbt_dynar_get_ptr(path_src, max)))->name,
                                   (*(AS_t *)
                                    (xbt_dynar_get_ptr(path_dst, i)))->name);
        e_route_bypass = xbt_dict_get_or_null(dict_bypassRoutes, route_name);
        xbt_free(route_name);
      }
      if (e_route_bypass)
        break;
    }

    if (e_route_bypass)
      break;

    if (max <= max_index_src && max <= max_index_dst) {
      char *route_name = bprintf("%s#%s",
                                 (*(AS_t *)
                                  (xbt_dynar_get_ptr(path_src, max)))->name,
                                 (*(AS_t *)
                                  (xbt_dynar_get_ptr(path_dst, max)))->name);
      e_route_bypass = xbt_dict_get_or_null(dict_bypassRoutes, route_name);
      xbt_free(route_name);
    }
    if (e_route_bypass)
      break;
  }

  xbt_dynar_free(&path_src);
  xbt_dynar_free(&path_dst);

  route_t new_e_route = NULL;

  if (e_route_bypass) {
    void *link;
    unsigned int cpt = 0;
    new_e_route = xbt_new0(s_route_t, 1);
    new_e_route->src_gateway = e_route_bypass->src_gateway;
    new_e_route->dst_gateway = e_route_bypass->dst_gateway;
    new_e_route->link_list =
        xbt_dynar_new(global_routing->size_of_link, NULL);
    xbt_dynar_foreach(e_route_bypass->link_list, cpt, link) {
      xbt_dynar_push(new_e_route->link_list, &link);
    }
  }

  return new_e_route;
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */

route_t
generic_new_route(e_surf_routing_hierarchy_t hierarchy, void *data, int order)
{

  char *link_name;
  route_t new_route;
  unsigned int cpt;
  xbt_dynar_t links = NULL, links_id = NULL;

  new_route = xbt_new0(s_route_t, 1);
  new_route->link_list = xbt_dynar_new(global_routing->size_of_link, NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE,
             "the hierarchy type is not SURF_ROUTING_BASE");

  links = ((route_t) data)->link_list;


  links_id = new_route->link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROWF(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_route;
}

route_t
generic_new_extended_route(e_surf_routing_hierarchy_t hierarchy,
                           void *data, int order)
{

  char *link_name;
  route_t e_route, new_e_route;
  route_t route;
  unsigned int cpt;
  xbt_dynar_t links = NULL, links_id = NULL;

  new_e_route = xbt_new0(s_route_t, 1);
  new_e_route->link_list = xbt_dynar_new(global_routing->size_of_link, NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE
             || hierarchy == SURF_ROUTING_RECURSIVE,
             "the hierarchy type is not defined");

  if (hierarchy == SURF_ROUTING_BASE) {

    route = (route_t) data;
    links = route->link_list;

  } else if (hierarchy == SURF_ROUTING_RECURSIVE) {

    e_route = (route_t) data;
    xbt_assert(e_route->src_gateway
               && e_route->dst_gateway, "bad gateway, is null");
    links = e_route->link_list;

    /* remeber not erase the gateway names */
    new_e_route->src_gateway = e_route->src_gateway;
    new_e_route->dst_gateway = e_route->dst_gateway;
  }

  links_id = new_e_route->link_list;

  xbt_dynar_foreach(links, cpt, link_name) {

    void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
    if (link) {
      if (order)
        xbt_dynar_push(links_id, &link);
      else
        xbt_dynar_unshift(links_id, &link);
    } else
      THROWF(mismatch_error, 0, "Link %s not found", link_name);
  }

  return new_e_route;
}

void generic_free_route(route_t route)
{
  if (route) {
    xbt_dynar_free(&route->link_list);
    xbt_free(route);
  }
}

static AS_t generic_as_exist(AS_t find_from,
                                            AS_t to_find)
{
  //return to_find; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  int found = 0;
  AS_t elem;
  xbt_dict_foreach(find_from->routing_sons, cursor, key, elem) {
    if (to_find == elem || generic_as_exist(elem, to_find)) {
      found = 1;
      break;
    }
  }
  if (found)
    return to_find;
  return NULL;
}

AS_t
generic_autonomous_system_exist(AS_t rc, char *element)
{
  //return rc; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  AS_t element_as, result, elem;
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  element_as = ((network_element_t)
                xbt_lib_get_or_null(as_router_lib, element,
                                    ROUTING_ASR_LEVEL))->rc_component;
  result = ((AS_t) - 1);
  if (element_as != rc)
    result = generic_as_exist(rc, element_as);

  int found = 0;
  if (result) {
    xbt_dict_foreach(element_as->routing_sons, cursor, key, elem) {
      found = !strcmp(elem->name, element);
      if (found)
        break;
    }
    if (found)
      return element_as;
  }
  return NULL;
}

AS_t
generic_processing_units_exist(AS_t rc, char *element)
{
  AS_t element_as;
  element_as = ((network_element_t)
                xbt_lib_get_or_null(host_lib,
                                    element, ROUTING_HOST_LEVEL))->rc_component;
  if (element_as == rc)
    return element_as;
  return generic_as_exist(rc, element_as);
}

void generic_src_dst_check(AS_t rc, network_element_t src,
    network_element_t dst)
{

  network_element_t src_data = src;
  network_element_t dst_data = dst;

  if (src_data == NULL || dst_data == NULL)
    xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
        src->name,
        dst->name,
        rc->name);

  AS_t src_as =
      (src_data)->rc_component;
  AS_t dst_as =
      (dst_data)->rc_component;

  if (src_as != dst_as)
    xbt_die("The src(%s in %s) and dst(%s in %s) are in differents AS",
        src->name, src_as->name,
        dst->name, dst_as->name);

  if (rc != dst_as)
    xbt_die
        ("The routing component of src'%s' and dst'%s' is not the same as the network elements belong (%s?=%s?=%s)",
            src->name,
            dst->name,
            src_as->name,
            dst_as->name,
            rc->name);
}
