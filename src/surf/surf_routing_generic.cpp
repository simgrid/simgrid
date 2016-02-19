/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <algorithm>

#include <xbt/dict.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>
#include <xbt/dynar.h>
#include <xbt/graph.h>

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_generic.hpp"
#include "surf_routing_private.hpp"
#include "network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");

static int no_bypassroute_declared = 1;

void routing_route_free(sg_platf_route_cbarg_t route)
{
  if (route) {
    xbt_dynar_free(&route->link_list);
    xbt_free(route);
  }
}

namespace simgrid {
namespace surf {
  
AsGeneric::AsGeneric(const char*name)
  : As(name)
{
}

AsGeneric::~AsGeneric()
{
  xbt_dict_free(&bypassRoutes_);
}

void AsGeneric::parseBypassroute(sg_platf_route_cbarg_t e_route)
{
  char *src = (char*)(e_route->src);
  char *dst = (char*)(e_route->dst);

  if(e_route->gw_dst)
    XBT_DEBUG("Load bypassASroute from \"%s\" to \"%s\"", src, dst);
  else
    XBT_DEBUG("Load bypassRoute from \"%s\" to \"%s\"", src, dst);
  xbt_dict_t dict_bypassRoutes = bypassRoutes_;
  char *route_name;

  route_name = bprintf("%s#%s", src, dst);
  xbt_assert(!xbt_dynar_is_empty(e_route->link_list),
      "Invalid count of links, must be greater than zero (%s,%s)",
      src, dst);
  xbt_assert(!xbt_dict_get_or_null(dict_bypassRoutes, route_name),
      "The bypass route between \"%s\"(\"%s\") and \"%s\"(\"%s\") already exists",
      src, e_route->gw_src->name(), dst, e_route->gw_dst->name());

  sg_platf_route_cbarg_t new_e_route = NULL;
  if(e_route->gw_dst)
    new_e_route =  newExtendedRoute(SURF_ROUTING_RECURSIVE, e_route, 1);
  else
    new_e_route =  newExtendedRoute(SURF_ROUTING_BASE, e_route, 1);

  xbt_dynar_free(&(e_route->link_list));

  xbt_dict_set(dict_bypassRoutes, route_name, new_e_route, NULL);
  no_bypassroute_declared = 0;
  xbt_free(route_name);
}

}
}

/* ************************************************************************** */
/* *********************** GENERIC BUSINESS METHODS ************************* */

static const char *instr_node_name(xbt_node_t node)
{
  void *data = xbt_graph_node_get_data(node);
  char *str = (char *) data;
  return str;
}

xbt_node_t new_xbt_graph_node(xbt_graph_t graph, const char *name,
                              xbt_dict_t nodes)
{
  xbt_node_t ret = (xbt_node_t) xbt_dict_get_or_null(nodes, name);
  if (ret)
    return ret;

  ret = xbt_graph_new_node(graph, xbt_strdup(name));
  xbt_dict_set(nodes, name, ret, NULL);
  return ret;
}

xbt_edge_t new_xbt_graph_edge(xbt_graph_t graph, xbt_node_t s, xbt_node_t d,
                              xbt_dict_t edges)
{
  xbt_edge_t ret;

  const char *sn = instr_node_name(s);
  const char *dn = instr_node_name(d);
  int len = strlen(sn) + strlen(dn) + 1;
  char *name = (char *) xbt_malloc(len * sizeof(char));


  snprintf(name, len, "%s%s", sn, dn);
  ret = (xbt_edge_t) xbt_dict_get_or_null(edges, name);
  if (ret == NULL) {
    snprintf(name, len, "%s%s", dn, sn);
    ret = (xbt_edge_t) xbt_dict_get_or_null(edges, name);
  }

  if (ret == NULL) {
    ret = xbt_graph_new_edge(graph, s, d, NULL);
    xbt_dict_set(edges, name, ret, NULL);
  }
  free(name);
  return ret;
}

namespace simgrid {
namespace surf {

void AsGeneric::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  int src, dst;
  int table_size = xbt_dynar_length(vertices_);


  for (src = 0; src < table_size; src++) {
    NetCard *my_src =
        xbt_dynar_get_as(vertices_, src, NetCard*);
    for (dst = 0; dst < table_size; dst++) {
      if (src == dst)
        continue;
      NetCard *my_dst =
          xbt_dynar_get_as(vertices_, dst, NetCard*);

      sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
      route->link_list = xbt_dynar_new(sizeof(Link*), NULL);

      getRouteAndLatency(my_src, my_dst, route, NULL);

      XBT_DEBUG ("get_route_and_latency %s -> %s", my_src->name(), my_dst->name());

      unsigned int cpt;
      void *link;

      xbt_node_t current, previous;
      const char *previous_name, *current_name;

      if (route->gw_src) {
        previous = new_xbt_graph_node(graph, route->gw_src->name(), nodes);
        previous_name = route->gw_src->name();
      } else {
        previous = new_xbt_graph_node(graph, my_src->name(), nodes);
        previous_name = my_src->name();
      }

      xbt_dynar_foreach(route->link_list, cpt, link) {
        const char *link_name = static_cast<simgrid::surf::Resource*>(
          link)->getName();
        current = new_xbt_graph_node(graph, link_name, nodes);
        current_name = link_name;
        new_xbt_graph_edge(graph, previous, current, edges);
        XBT_DEBUG ("  %s -> %s", previous_name, current_name);
        previous = current;
        previous_name = current_name;
      }

      if (route->gw_dst) {
        current = new_xbt_graph_node(graph, route->gw_dst->name(), nodes);
        current_name = route->gw_dst->name();
      } else {
        current = new_xbt_graph_node(graph, my_dst->name(), nodes);
        current_name = my_dst->name();
      }
      new_xbt_graph_edge(graph, previous, current, edges);
      XBT_DEBUG ("  %s -> %s", previous_name, current_name);

      xbt_dynar_free (&(route->link_list));
      xbt_free (route);
    }
  }
}

sg_platf_route_cbarg_t AsGeneric::getBypassRoute(NetCard *src,
                                               NetCard *dst,
                                               double *lat)
{
  // If never set a bypass route return NULL without any further computations
  XBT_DEBUG("generic_get_bypassroute from %s to %s", src->name(), dst->name());
  if (no_bypassroute_declared)
    return NULL;

  sg_platf_route_cbarg_t e_route_bypass = NULL;
  xbt_dict_t dict_bypassRoutes = bypassRoutes_;

  if(dst->containingAS() == this && src->containingAS() == this ){
    char *route_name = bprintf("%s#%s", src->name(), dst->name());
    e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
    if(e_route_bypass)
      XBT_DEBUG("Find bypass route with %ld links",xbt_dynar_length(e_route_bypass->link_list));
    free(route_name);
  }
  else{
    As *src_as, *dst_as;
    int index_src, index_dst;
    xbt_dynar_t path_src = NULL;
    xbt_dynar_t path_dst = NULL;
    As *current = NULL;
    As **current_src = NULL;
    As **current_dst = NULL;

    if (src == NULL || dst == NULL)
      xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
              src ? src->name() : "(null)",
              dst ? dst->name() : "(null)", name_);

    src_as = src->containingAS();
    dst_as = dst->containingAS();

    /* (2) find the path to the root routing component */
    path_src = xbt_dynar_new(sizeof(As*), NULL);
    current = src_as;
    while (current != NULL) {
      xbt_dynar_push(path_src, &current);
      current = current->father_;
    }
    path_dst = xbt_dynar_new(sizeof(As*), NULL);
    current = dst_as;
    while (current != NULL) {
      xbt_dynar_push(path_dst, &current);
      current = current->father_;
    }

    /* (3) find the common father */
    index_src = path_src->used - 1;
    index_dst = path_dst->used - 1;
    current_src = (As **) xbt_dynar_get_ptr(path_src, index_src);
    current_dst = (As **) xbt_dynar_get_ptr(path_dst, index_dst);
    while (index_src >= 0 && index_dst >= 0 && *current_src == *current_dst) {
      xbt_dynar_pop_ptr(path_src);
      xbt_dynar_pop_ptr(path_dst);
      index_src--;
      index_dst--;
      current_src = (As **) xbt_dynar_get_ptr(path_src, index_src);
      current_dst = (As **) xbt_dynar_get_ptr(path_dst, index_dst);
    }

    int max_index_src = path_src->used - 1;
    int max_index_dst = path_dst->used - 1;

    int max_index = std::max(max_index_src, max_index_dst);
    int i, max;

    for (max = 0; max <= max_index; max++) {
      for (i = 0; i < max; i++) {
        if (i <= max_index_src && max <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(As **)
                  (xbt_dynar_get_ptr(path_src, i)))->name_,
                  (*(As **)
                      (xbt_dynar_get_ptr(path_dst, max)))->name_);
          e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
          xbt_free(route_name);
        }
        if (e_route_bypass)
          break;
        if (max <= max_index_src && i <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(As **)
                  (xbt_dynar_get_ptr(path_src, max)))->name_,
                  (*(As **)
                      (xbt_dynar_get_ptr(path_dst, i)))->name_);
          e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
          xbt_free(route_name);
        }
        if (e_route_bypass)
          break;
      }

      if (e_route_bypass)
        break;

      if (max <= max_index_src && max <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
            (*(As **)
                (xbt_dynar_get_ptr(path_src, max)))->name_,
                (*(As **)
                    (xbt_dynar_get_ptr(path_dst, max)))->name_);
        e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
        xbt_free(route_name);
      }
      if (e_route_bypass)
        break;
    }

    xbt_dynar_free(&path_src);
    xbt_dynar_free(&path_dst);
  }

  sg_platf_route_cbarg_t new_e_route = NULL;
  if (e_route_bypass) {
    Link* link;
    unsigned int cpt = 0;
    new_e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
    new_e_route->gw_src = e_route_bypass->gw_src;
    new_e_route->gw_dst = e_route_bypass->gw_dst;
    new_e_route->link_list =
        xbt_dynar_new(sizeof(Link*), NULL);
    xbt_dynar_foreach(e_route_bypass->link_list, cpt, link) {
      xbt_dynar_push(new_e_route->link_list, &link);
      if (lat)
        *lat += link->getLatency();
    }
  }

  return new_e_route;
}

/* ************************************************************************** */
/* ************************* GENERIC AUX FUNCTIONS ************************** */
/* change a route containing link names into a route containing link entities */
sg_platf_route_cbarg_t AsGeneric::newExtendedRoute(e_surf_routing_hierarchy_t hierarchy,
      sg_platf_route_cbarg_t routearg, int change_order) {

  sg_platf_route_cbarg_t result;
  char *link_name;
  unsigned int cpt;

  result = xbt_new0(s_sg_platf_route_cbarg_t, 1);
  result->link_list = xbt_dynar_new(sizeof(Link*), NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE
      || hierarchy == SURF_ROUTING_RECURSIVE,
      "The hierarchy of this AS is neither BASIC nor RECURSIVE, I'm lost here.");

  if (hierarchy == SURF_ROUTING_RECURSIVE) {

    xbt_assert(routearg->gw_src && routearg->gw_dst, "NULL is obviously a deficient gateway");

    /* remember not erase the gateway names */
    result->gw_src = routearg->gw_src;
    result->gw_dst = routearg->gw_dst;
  }

  xbt_dynar_foreach(routearg->link_list, cpt, link_name) {

    Link *link = Link::byName(link_name);
    if (link) {
      if (change_order)
        xbt_dynar_push(result->link_list, &link);
      else
        xbt_dynar_unshift(result->link_list, &link);
    } else
      THROWF(mismatch_error, 0, "Link '%s' not found", link_name);
  }

  return result;
}

void AsGeneric::getRouteCheckParams(NetCard *src, NetCard *dst)
{
  xbt_assert(src,"Cannot find a route from NULL to %s", dst->name());
  xbt_assert(dst,"Cannot find a route from %s to NULL", src->name());

  As *src_as = src->containingAS();
  As *dst_as = dst->containingAS();

  xbt_assert(src_as == dst_as, "Internal error: %s@%s and %s@%s are not in the same AS as expected. Please report that bug.",
        src->name(), src_as->name_, dst->name(), dst_as->name_);

  xbt_assert(this == dst_as,
      "Internal error: route destination %s@%s is not in AS %s as expected (route source: %s@%s). Please report that bug.",
        src->name(), dst->name(),  src_as->name_, dst_as->name_,  name_);
}
void AsGeneric::addRouteCheckParams(sg_platf_route_cbarg_t route) {
  const char *srcName = route->src;
  const char *dstName = route->dst;
  NetCard *src = sg_netcard_by_name_or_null(srcName);
  NetCard *dst = sg_netcard_by_name_or_null(dstName);

  if(!route->gw_dst && !route->gw_src) {
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", srcName, dstName);
    xbt_assert(src, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, srcName);
    xbt_assert(dst, "Cannot add a route from %s to %s: %s does not exist.", srcName, dstName, dstName);
    xbt_assert(!xbt_dynar_is_empty(route->link_list), "Empty route (between %s and %s) forbidden.", srcName, dstName);
    xbt_assert(src->getRcType()==SURF_NETWORK_ELEMENT_HOST || src->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining a route, src must be an host or a router but '%s' is not. Did you meant to have an ASroute?", srcName);
    xbt_assert(dst->getRcType()==SURF_NETWORK_ELEMENT_HOST || dst->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining a route, dst must be an host or a router but '%s' is not. Did you meant to have an ASroute?", dstName);
  } else {
    XBT_DEBUG("Load ASroute from %s@%s to %s@%s", srcName, route->gw_src->name(), dstName, route->gw_dst->name());
    xbt_assert(src->getRcType()==SURF_NETWORK_ELEMENT_AS,
        "When defining an ASroute, src must be an AS but '%s' is not", srcName);
    xbt_assert(dst->getRcType()==SURF_NETWORK_ELEMENT_AS,
        "When defining an ASroute, dst must be an AS but '%s' is not", dstName);

    xbt_assert(route->gw_src->getRcType()==SURF_NETWORK_ELEMENT_HOST || route->gw_src->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining an ASroute, gw_src must be an host or a router but '%s' is not.", srcName);
    xbt_assert(route->gw_dst->getRcType()==SURF_NETWORK_ELEMENT_HOST || route->gw_dst->getRcType()==SURF_NETWORK_ELEMENT_ROUTER,
        "When defining an ASroute, gw_dst must be an host or a router but '%s' is not.", dstName);

    xbt_assert(route->gw_src != route->gw_dst, "Cannot define an ASroute from '%s' to itself", route->gw_src->name());

    xbt_assert(src, "Cannot add a route from %s@%s to %s@%s: %s does not exist.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name(), srcName);
    xbt_assert(dst, "Cannot add a route from %s@%s to %s@%s: %s does not exist.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name(), dstName);
    xbt_assert(!xbt_dynar_is_empty(route->link_list), "Empty route (between %s@%s and %s@%s) forbidden.",
        srcName,route->gw_src->name(), dstName,route->gw_dst->name());
  }
}

}
}
