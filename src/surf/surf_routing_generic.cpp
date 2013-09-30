/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/platf_interface.h"    // platform creation API internal interface

#include "surf_routing_generic.hpp"
#include "network.hpp"
#include "xbt/graph.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_routing_generic, surf_route, "Generic implementation of the surf routing");
}

static int no_bypassroute_declared = 1;

void generic_free_route(sg_platf_route_cbarg_t route)
{
  if (route) {
    xbt_dynar_free(&route->link_list);
    xbt_free(route);
  }
}

void AsGeneric::parseRoute(sg_platf_route_cbarg_t route){
  THROW_IMPOSSIBLE;
}

void AsGeneric::parseASroute(sg_platf_route_cbarg_t route){
  THROW_IMPOSSIBLE;
}

void AsGeneric::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency){
  THROW_IMPOSSIBLE;
}

xbt_dynar_t AsGeneric::getOneLinkRoutes(){
  THROW_IMPOSSIBLE;
}

AsGeneric::AsGeneric() {
  p_bypassRoutes = xbt_dict_new_homogeneous((void (*)(void *)) generic_free_route);
}

AsGeneric::~AsGeneric() {
  xbt_dict_free(&p_bypassRoutes);
}

int AsGeneric::parsePU(RoutingEdgePtr elm)
{
  XBT_DEBUG("Load process unit \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  return xbt_dynar_length(p_indexNetworkElm)-1;
}

int AsGeneric::parseAS(RoutingEdgePtr elm)
{
  XBT_DEBUG("Load Autonomous system \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, RoutingEdgePtr, elm);
  return xbt_dynar_length(p_indexNetworkElm)-1;
}

void AsGeneric::parseBypassroute(sg_platf_route_cbarg_t e_route)
{
  char *src = (char*)(e_route->src);
  char *dst = (char*)(e_route->dst);

  if(e_route->gw_dst)
    XBT_DEBUG("Load bypassASroute from \"%s\" to \"%s\"", src, dst);
  else
    XBT_DEBUG("Load bypassRoute from \"%s\" to \"%s\"", src, dst);
  xbt_dict_t dict_bypassRoutes = p_bypassRoutes;
  char *route_name;

  route_name = bprintf("%s#%s", src, dst);
  xbt_assert(!xbt_dynar_is_empty(e_route->link_list),
      "Invalid count of links, must be greater than zero (%s,%s)",
      src, dst);
  xbt_assert(!xbt_dict_get_or_null(dict_bypassRoutes, route_name),
      "The bypass route between \"%s\"(\"%s\") and \"%s\"(\"%s\") already exists",
      src, e_route->gw_src->p_name, dst, e_route->gw_dst->p_name);

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

/* ************************************************************************** */
/* *********************** GENERIC BUSINESS METHODS ************************* */

xbt_dynar_t AsGeneric::getOnelinkRoutes() { // FIXME: kill that stub
  xbt_die("\"generic_get_onelink_routes\" not implemented yet");
  return NULL;
}

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

void AsGeneric::getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges)
{
  int src, dst;
  int table_size = xbt_dynar_length(p_indexNetworkElm);


  for (src = 0; src < table_size; src++) {
    RoutingEdgePtr my_src =
        xbt_dynar_get_as(p_indexNetworkElm, src, RoutingEdgePtr);
    for (dst = 0; dst < table_size; dst++) {
      if (src == dst)
        continue;
      RoutingEdgePtr my_dst =
          xbt_dynar_get_as(p_indexNetworkElm, dst, RoutingEdgePtr);

      sg_platf_route_cbarg_t route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
      route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);

      getRouteAndLatency(my_src, my_dst, route, NULL);

      XBT_DEBUG ("get_route_and_latency %s -> %s", my_src->p_name, my_dst->p_name);

      unsigned int cpt;
      void *link;

      xbt_node_t current, previous;
      const char *previous_name, *current_name;

      if (route->gw_src) {
        previous = new_xbt_graph_node(graph, route->gw_src->p_name, nodes);
        previous_name = route->gw_src->p_name;
      } else {
        previous = new_xbt_graph_node(graph, my_src->p_name, nodes);
        previous_name = my_src->p_name;
      }

      xbt_dynar_foreach(route->link_list, cpt, link) {
        const char *link_name = ((ResourcePtr) link)->m_name;
        current = new_xbt_graph_node(graph, link_name, nodes);
        current_name = link_name;
        new_xbt_graph_edge(graph, previous, current, edges);
        XBT_DEBUG ("  %s -> %s", previous_name, current_name);
        previous = current;
        previous_name = current_name;
      }

      if (route->gw_dst) {
        current = new_xbt_graph_node(graph, route->gw_dst->p_name, nodes);
        current_name = route->gw_dst->p_name;
      } else {
        current = new_xbt_graph_node(graph, my_dst->p_name, nodes);
        current_name = my_dst->p_name;
      }
      new_xbt_graph_edge(graph, previous, current, edges);
      XBT_DEBUG ("  %s -> %s", previous_name, current_name);

      xbt_dynar_free (&(route->link_list));
      xbt_free (route);
    }
  }
}

sg_platf_route_cbarg_t AsGeneric::getBypassRoute(RoutingEdgePtr src,
                                               RoutingEdgePtr dst,
                                               double *lat)
{
  // If never set a bypass route return NULL without any further computations
  XBT_DEBUG("generic_get_bypassroute from %s to %s", src->p_name, dst->p_name);
  if (no_bypassroute_declared)
    return NULL;

  sg_platf_route_cbarg_t e_route_bypass = NULL;
  xbt_dict_t dict_bypassRoutes = p_bypassRoutes;

  if(dst->p_rcComponent == this && src->p_rcComponent == this ){
    char *route_name = bprintf("%s#%s", src->p_name, dst->p_name);
    e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
    if(e_route_bypass)
      XBT_DEBUG("Find bypass route with %ld links",xbt_dynar_length(e_route_bypass->link_list));
    free(route_name);
  }
  else{
    AsPtr src_as, dst_as;
    int index_src, index_dst;
    xbt_dynar_t path_src = NULL;
    xbt_dynar_t path_dst = NULL;
    AsPtr current = NULL;
    AsPtr *current_src = NULL;
    AsPtr *current_dst = NULL;

    if (src == NULL || dst == NULL)
      xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
          src->p_name, dst->p_name, p_name);

    src_as = src->p_rcComponent;
    dst_as = dst->p_rcComponent;

    /* (2) find the path to the root routing component */
    path_src = xbt_dynar_new(sizeof(AsPtr), NULL);
    current = src_as;
    while (current != NULL) {
      xbt_dynar_push(path_src, &current);
      current = current->p_routingFather;
    }
    path_dst = xbt_dynar_new(sizeof(AsPtr), NULL);
    current = dst_as;
    while (current != NULL) {
      xbt_dynar_push(path_dst, &current);
      current = current->p_routingFather;
    }

    /* (3) find the common father */
    index_src = path_src->used - 1;
    index_dst = path_dst->used - 1;
    current_src = (AsPtr *) xbt_dynar_get_ptr(path_src, index_src);
    current_dst = (AsPtr *) xbt_dynar_get_ptr(path_dst, index_dst);
    while (index_src >= 0 && index_dst >= 0 && *current_src == *current_dst) {
      xbt_dynar_pop_ptr(path_src);
      xbt_dynar_pop_ptr(path_dst);
      index_src--;
      index_dst--;
      current_src = (AsPtr *) xbt_dynar_get_ptr(path_src, index_src);
      current_dst = (AsPtr *) xbt_dynar_get_ptr(path_dst, index_dst);
    }

    int max_index_src = path_src->used - 1;
    int max_index_dst = path_dst->used - 1;

    int max_index = max(max_index_src, max_index_dst);
    int i, max;

    for (max = 0; max <= max_index; max++) {
      for (i = 0; i < max; i++) {
        if (i <= max_index_src && max <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(AsPtr *)
                  (xbt_dynar_get_ptr(path_src, i)))->p_name,
                  (*(AsPtr *)
                      (xbt_dynar_get_ptr(path_dst, max)))->p_name);
          e_route_bypass = (sg_platf_route_cbarg_t) xbt_dict_get_or_null(dict_bypassRoutes, route_name);
          xbt_free(route_name);
        }
        if (e_route_bypass)
          break;
        if (max <= max_index_src && i <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(AsPtr *)
                  (xbt_dynar_get_ptr(path_src, max)))->p_name,
                  (*(AsPtr *)
                      (xbt_dynar_get_ptr(path_dst, i)))->p_name);
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
            (*(AsPtr *)
                (xbt_dynar_get_ptr(path_src, max)))->p_name,
                (*(AsPtr *)
                    (xbt_dynar_get_ptr(path_dst, max)))->p_name);
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
    NetworkCm02LinkPtr link;
    unsigned int cpt = 0;
    new_e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
    new_e_route->gw_src = e_route_bypass->gw_src;
    new_e_route->gw_dst = e_route_bypass->gw_dst;
    new_e_route->link_list =
        xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
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
  result->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);

  xbt_assert(hierarchy == SURF_ROUTING_BASE
      || hierarchy == SURF_ROUTING_RECURSIVE,
      "The hierarchy of this AS is neither BASIC nor RECURSIVE, I'm lost here.");

  if (hierarchy == SURF_ROUTING_RECURSIVE) {

    xbt_assert(routearg->gw_src && routearg->gw_dst,
        "NULL is obviously a bad gateway");

    /* remeber not erase the gateway names */
    result->gw_src = routearg->gw_src;
    result->gw_dst = routearg->gw_dst;
  }

  xbt_dynar_foreach(routearg->link_list, cpt, link_name) {

    void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
    if (link) {
      if (change_order)
        xbt_dynar_push(result->link_list, &link);
      else
        xbt_dynar_unshift(result->link_list, &link);
    } else
      THROWF(mismatch_error, 0, "Link %s not found", link_name);
  }

  return result;
}



AsPtr AsGeneric::asExist(AsPtr to_find)
{
  //return to_find; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  int found = 0;
  AsGenericPtr elem;
  xbt_dict_foreach(p_routingSons, cursor, key, elem) {
    if (to_find == elem || elem->asExist(to_find)) {
      found = 1;
      break;
    }
  }
  if (found)
    return to_find;
  return NULL;
}

AsPtr AsGeneric::autonomousSystemExist(char *element)
{
  //return rc; // FIXME: BYPASSERROR OF FOREACH WITH BREAK
  AsPtr element_as, result, elem;
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  element_as = ((RoutingEdgePtr)
      xbt_lib_get_or_null(as_router_lib, element,
          ROUTING_ASR_LEVEL))->p_rcComponent;
  result = ((AsPtr) - 1);
  if (element_as != this)
    result = asExist(element_as);

  int found = 0;
  if (result) {
    xbt_dict_foreach(element_as->p_routingSons, cursor, key, elem) {
      found = !strcmp(elem->p_name, element);
      if (found)
        break;
    }
    if (found)
      return element_as;
  }
  return NULL;
}

AsPtr AsGeneric::processingUnitsExist(char *element)
{
  AsPtr element_as;
  element_as = ((RoutingEdgePtr)
      xbt_lib_get_or_null(host_lib,
          element, ROUTING_HOST_LEVEL))->p_rcComponent;
  if (element_as == this)
    return element_as;
  return asExist(element_as);
}

void AsGeneric::srcDstCheck(RoutingEdgePtr src, RoutingEdgePtr dst)
{

  RoutingEdgePtr src_data = src;
  RoutingEdgePtr dst_data = dst;

  if (src_data == NULL || dst_data == NULL)
    xbt_die("Ask for route \"from\"(%s) or \"to\"(%s) no found at AS \"%s\"",
        src->p_name,
        dst->p_name,
        p_name);

  AsPtr src_as =
      (src_data)->p_rcComponent;
  AsPtr dst_as =
      (dst_data)->p_rcComponent;

  if (src_as != dst_as)
    xbt_die("The src(%s in %s) and dst(%s in %s) are in differents AS",
        src->p_name, src_as->p_name,
        dst->p_name, dst_as->p_name);

  if (this != dst_as)
    xbt_die
    ("The routing component of src'%s' and dst'%s' is not the same as the network elements belong (%s?=%s?=%s)",
        src->p_name,
        dst->p_name,
        src_as->p_name,
        dst_as->p_name,
        p_name);
}
