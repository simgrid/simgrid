/* Copyright (c) 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing_private.h"

/* Global vars */
extern routing_platf_t routing_platf;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_floyd, surf, "Routing part of surf");

#define TO_FLOYD_COST(i,j) (as->cost_table)[(i)+(j)*table_size]
#define TO_FLOYD_PRED(i,j) (as->predecessor_table)[(i)+(j)*table_size]
#define TO_FLOYD_LINK(i,j) (as->link_table)[(i)+(j)*table_size]

/* Routing model structure */

typedef struct {
  s_as_t generic_routing;
  /* vars for calculate the floyd algorith. */
  int *predecessor_table;
  double *cost_table;
  sg_platf_route_cbarg_t *link_table;
} s_as_floyd_t, *as_floyd_t;

static void floyd_get_route_and_latency(AS_t asg, sg_routing_edge_t src, sg_routing_edge_t dst,
    sg_platf_route_cbarg_t res, double *lat);

/* Business methods */
static xbt_dynar_t floyd_get_onelink_routes(AS_t asg)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);
  sg_platf_route_cbarg_t route =   xbt_new0(s_sg_platf_route_cbarg_t, 1);
  route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);

  int src,dst;
  sg_routing_edge_t src_elm, dst_elm;
  int table_size = xbt_dynar_length(asg->index_network_elm);
  for(src=0; src < table_size; src++) {
    for(dst=0; dst< table_size; dst++) {
      xbt_dynar_reset(route->link_list);
      src_elm = xbt_dynar_get_as(asg->index_network_elm,src,sg_routing_edge_t);
      dst_elm = xbt_dynar_get_as(asg->index_network_elm,dst,sg_routing_edge_t);
      floyd_get_route_and_latency(asg, src_elm, dst_elm, route, NULL);

      if (xbt_dynar_length(route->link_list) == 1) {
        void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
        onelink_t onelink = xbt_new0(s_onelink_t, 1);
        onelink->link_ptr = link;
        if (asg->hierarchy == SURF_ROUTING_BASE) {
          onelink->src = src_elm;
          onelink->dst = dst_elm;
        } else if (asg->hierarchy == SURF_ROUTING_RECURSIVE) {
          onelink->src = route->gw_src;
          onelink->dst = route->gw_dst;
        }
        xbt_dynar_push(ret, &onelink);
      }
    }
  }

  return ret;
}

static void floyd_get_route_and_latency(AS_t asg, sg_routing_edge_t src, sg_routing_edge_t dst,
    sg_platf_route_cbarg_t res, double *lat)
{

  /* set utils vars */
  as_floyd_t as = (as_floyd_t)asg;
  size_t table_size = xbt_dynar_length(asg->index_network_elm);

  generic_src_dst_check(asg, src, dst);

  /* create a result route */
  xbt_dynar_t route_stack = xbt_dynar_new(sizeof(sg_platf_route_cbarg_t), NULL);
  int pred;
  int cur = dst->id;
  do {
    pred = TO_FLOYD_PRED(src->id, cur);
    if (pred == -1)
      THROWF(arg_error, 0, "No route from '%s' to '%s'", src->name, dst->name);
    xbt_dynar_push_as(route_stack, sg_platf_route_cbarg_t, TO_FLOYD_LINK(pred, cur));
    cur = pred;
  } while (cur != src->id);

  if (asg->hierarchy == SURF_ROUTING_RECURSIVE) {
    res->gw_src = xbt_dynar_getlast_as(route_stack, sg_platf_route_cbarg_t)->gw_src;
    res->gw_dst = xbt_dynar_getfirst_as(route_stack, sg_platf_route_cbarg_t)->gw_dst;
  }

  sg_routing_edge_t prev_dst_gw = NULL;
  while (!xbt_dynar_is_empty(route_stack)) {
    sg_platf_route_cbarg_t e_route = xbt_dynar_pop_as(route_stack, sg_platf_route_cbarg_t);
    xbt_dynar_t links;
    sg_routing_link_t link;
    unsigned int cpt;

    if (asg->hierarchy == SURF_ROUTING_RECURSIVE && prev_dst_gw != NULL
        && strcmp(prev_dst_gw->name, e_route->gw_src->name)) {
      routing_get_route_and_latency(prev_dst_gw, e_route->gw_src,
                                    &res->link_list, lat);
    }

    links = e_route->link_list;
    xbt_dynar_foreach(links, cpt, link) {
      xbt_dynar_push_as(res->link_list, sg_routing_link_t, link);
      if (lat)
        *lat += surf_network_model->extension.network.get_link_latency(link);
    }

    prev_dst_gw = e_route->gw_dst;
  }
  xbt_dynar_free(&route_stack);
}

static void floyd_finalize(AS_t rc)
{
  as_floyd_t as = (as_floyd_t) rc;
  int i, j;
  size_t table_size;
  if (as) {
    table_size = xbt_dynar_length(as->generic_routing.index_network_elm);
    /* Delete link_table */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++)
        generic_free_route(TO_FLOYD_LINK(i, j));
    xbt_free(as->link_table);
    /* Delete bypass dict */
    xbt_dict_free(&as->generic_routing.bypassRoutes);
    /* Delete predecessor and cost table */
    xbt_free(as->predecessor_table);
    xbt_free(as->cost_table);

    model_generic_finalize(rc);
  }
}

AS_t model_floyd_create(void)
{
  as_floyd_t new_component = (as_floyd_t)model_generic_create_sized(sizeof(s_as_floyd_t));
  new_component->generic_routing.parse_route = model_floyd_parse_route;
  new_component->generic_routing.parse_ASroute = model_floyd_parse_route;
  new_component->generic_routing.get_route_and_latency = floyd_get_route_and_latency;
  new_component->generic_routing.get_onelink_routes =
      floyd_get_onelink_routes;
  new_component->generic_routing.get_graph = generic_get_graph;
  new_component->generic_routing.finalize = floyd_finalize;
  return (AS_t)new_component;
}

void model_floyd_end(AS_t current_routing)
{

  as_floyd_t as =
      ((as_floyd_t) current_routing);

  unsigned int i, j, a, b, c;

  /* set the size of table routing */
  size_t table_size = xbt_dynar_length(as->generic_routing.index_network_elm);

  if(!as->link_table)
  {
    /* Create Cost, Predecessor and Link tables */
    as->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
    as->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
    as->link_table = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);    /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
      }
  }

  /* Add the loopback if needed */
  if (routing_platf->loopback && current_routing->hierarchy == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      sg_platf_route_cbarg_t e_route = TO_FLOYD_LINK(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
        xbt_dynar_push(e_route->link_list, &routing_platf->loopback);
        TO_FLOYD_LINK(i, i) = e_route;
        TO_FLOYD_PRED(i, i) = i;
        TO_FLOYD_COST(i, i) = 1;
      }
    }
  }
  /* Calculate path costs */
  for (c = 0; c < table_size; c++) {
    for (a = 0; a < table_size; a++) {
      for (b = 0; b < table_size; b++) {
        if (TO_FLOYD_COST(a, c) < DBL_MAX && TO_FLOYD_COST(c, b) < DBL_MAX) {
          if (TO_FLOYD_COST(a, b) == DBL_MAX ||
              (TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b) <
                  TO_FLOYD_COST(a, b))) {
            TO_FLOYD_COST(a, b) =
                TO_FLOYD_COST(a, c) + TO_FLOYD_COST(c, b);
            TO_FLOYD_PRED(a, b) = TO_FLOYD_PRED(c, b);
          }
        }
      }
    }
  }
}

static int floyd_pointer_resource_cmp(const void *a, const void *b) {
  return a != b;
}

//FIXME: kill dupplicates in next function with full routing

void model_floyd_parse_route(AS_t rc, sg_platf_route_cbarg_t route)
{
  char *src = (char*)(route->src);
  char *dst = (char*)(route->dst);

  int as_route = 0;
  as_floyd_t as = (as_floyd_t) rc;

  /* set the size of table routing */
  size_t table_size = xbt_dynar_length(rc->index_network_elm);
  sg_routing_edge_t src_net_elm, dst_net_elm;

  src_net_elm = sg_routing_edge_by_name_or_null(src);
  dst_net_elm = sg_routing_edge_by_name_or_null(dst);

  xbt_assert(src_net_elm, "Network elements %s not found", src);
  xbt_assert(dst_net_elm, "Network elements %s not found", dst);

  if(!as->link_table)
  {
    int i,j;
    /* Create Cost, Predecessor and Link tables */
    as->cost_table = xbt_new0(double, table_size * table_size);       /* link cost from host to host */
    as->predecessor_table = xbt_new0(int, table_size * table_size);  /* predecessor host numbers */
    as->link_table = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);    /* actual link between src and dst */

    /* Initialize costs and predecessors */
    for (i = 0; i < table_size; i++)
      for (j = 0; j < table_size; j++) {
        TO_FLOYD_COST(i, j) = DBL_MAX;
        TO_FLOYD_PRED(i, j) = -1;
        TO_FLOYD_LINK(i, j) = NULL;       /* fixed, missing in the previous version */
      }
  }
  if(!route->gw_dst && !route->gw_src)
    XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
  else{
    as_route = 1;
    XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", src,
        route->gw_src->name, dst, route->gw_dst->name);
    if(route->gw_dst->rc_type == SURF_NETWORK_ELEMENT_NULL)
      xbt_die("The dst_gateway '%s' does not exist!",route->gw_dst->name);
    if(route->gw_src->rc_type == SURF_NETWORK_ELEMENT_NULL)
      xbt_die("The src_gateway '%s' does not exist!",route->gw_src->name);
  }

  if(TO_FLOYD_LINK(src_net_elm->id, dst_net_elm->id))
  {

    char * link_name;
    unsigned int cpt;
    xbt_dynar_t link_route_to_test = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
    xbt_dynar_foreach(route->link_list,cpt,link_name)
    {
      void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
      xbt_assert(link,"Link : '%s' doesn't exists.",link_name);
      xbt_dynar_push(link_route_to_test,&link);
    }
    xbt_assert(!xbt_dynar_compare(
        (void*)TO_FLOYD_LINK(src_net_elm->id, dst_net_elm->id)->link_list,
        (void*)link_route_to_test,
        (int_f_cpvoid_cpvoid_t) floyd_pointer_resource_cmp),
        "The route between \"%s\" and \"%s\" already exists", src,dst);
  }
  else
  {
    TO_FLOYD_LINK(src_net_elm->id, dst_net_elm->id) =
        generic_new_extended_route(rc->hierarchy, route, 1);
    TO_FLOYD_PRED(src_net_elm->id, dst_net_elm->id) = src_net_elm->id;
    TO_FLOYD_COST(src_net_elm->id, dst_net_elm->id) =
        ((TO_FLOYD_LINK(src_net_elm->id, dst_net_elm->id))->link_list)->used;   /* count of links, old model assume 1 */
  }

  if ( (route->symmetrical == TRUE && as_route == 0)
      || (route->symmetrical == TRUE && as_route == 1)
  )
  {
    if(TO_FLOYD_LINK(dst_net_elm->id, src_net_elm->id))
    {
      if(!route->gw_dst && !route->gw_src)
        XBT_DEBUG("See Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("See ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
            route->gw_src->name, src, route->gw_dst->name);
      char * link_name;
      unsigned int i;
      xbt_dynar_t link_route_to_test = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
      for(i=xbt_dynar_length(route->link_list) ;i>0 ;i--)
      {
        link_name = xbt_dynar_get_as(route->link_list,i-1,void *);
        void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
        xbt_assert(link,"Link : '%s' doesn't exists.",link_name);
        xbt_dynar_push(link_route_to_test,&link);
      }
      xbt_assert(!xbt_dynar_compare(
          (void*)TO_FLOYD_LINK(dst_net_elm->id, src_net_elm->id)->link_list,
          (void*)link_route_to_test,
          (int_f_cpvoid_cpvoid_t) floyd_pointer_resource_cmp),
          "The route between \"%s\" and \"%s\" already exists", src,dst);
    }
    else
    {
      if(route->gw_dst && route->gw_src)
      {
        sg_routing_edge_t gw_src = route->gw_src;
        sg_routing_edge_t gw_dst = route->gw_dst;
        route->gw_src = gw_dst;
        route->gw_dst = gw_src;
      }

      if(!route->gw_src && !route->gw_dst)
        XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"", dst,
            route->gw_src->name, src, route->gw_dst->name);

      TO_FLOYD_LINK(dst_net_elm->id, src_net_elm->id) =
          generic_new_extended_route(rc->hierarchy, route, 0);
      TO_FLOYD_PRED(dst_net_elm->id, src_net_elm->id) = dst_net_elm->id;
      TO_FLOYD_COST(dst_net_elm->id, src_net_elm->id) =
          ((TO_FLOYD_LINK(dst_net_elm->id, src_net_elm->id))->link_list)->used;   /* count of links, old model assume 1 */
    }
  }
  xbt_dynar_free(&route->link_list);
}
