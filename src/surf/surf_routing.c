/* Copyright (c) 2009 The SimGrid team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "xbt/dynar.h"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route,surf,"Routing part of surf");// FIXME: connect this under windows

typedef struct {
  s_routing_t generic_routing;
  xbt_dynar_t *routing_table;
  void *loopback;
  size_t size_of_link;
}s_routing_full_t,*routing_full_t;

routing_t used_routing = NULL;
/* ************************************************************************** */
/* *************************** FULL ROUTING ********************************* */

#define ROUTE_FULL(i,j) ((routing_full_t)used_routing)->routing_table[(i)+(j)*(used_routing)->host_count]

/*
 * Parsing
 */
static void routing_full_parse_Shost(void) {
  int *val = xbt_malloc(sizeof(int));
  DEBUG2("Seen host %s (#%d)",A_surfxml_host_id,used_routing->host_count);
  *val = used_routing->host_count++;
  xbt_dict_set(used_routing->host_id,A_surfxml_host_id,val,xbt_free);
}
static int src_id = -1;
static int dst_id = -1;
static void routing_full_parse_Sroute_set_endpoints(void)
{
  src_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_src);
  dst_id = *(int*)xbt_dict_get(used_routing->host_id,A_surfxml_route_dst);
  DEBUG4("Route %s %d -> %s %d",A_surfxml_route_src,src_id,A_surfxml_route_dst,dst_id);
  route_action = A_surfxml_route_action;
}
static void routing_full_parse_Eroute(void)
{
  char *name;
  if (src_id != -1 && dst_id != -1) {
    name = bprintf("%x#%x", src_id, dst_id);
    manage_route(route_table, name, route_action, 0);
    free(name);
  }
}


static void routing_full_parse_end(void) {
  routing_full_t routing = (routing_full_t) used_routing;
  int nb_link = 0;
  unsigned int cpt = 0;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data, *end;
  const char *sep = "#";
  xbt_dynar_t links, keys;
  char *link_name = NULL;
  int i,j;

  int host_count = routing->generic_routing.host_count;

  /* Create the routing table */
  routing->routing_table = xbt_new0(xbt_dynar_t, host_count * host_count);
  for (i=0;i<host_count;i++)
    for (j=0;j<host_count;j++)
      ROUTE_FULL(i,j) = xbt_dynar_new(routing->size_of_link,NULL);

  /* Put the routes in position */
  xbt_dict_foreach(route_table, cursor, key, data) {
    nb_link = 0;
    links = (xbt_dynar_t) data;
    keys = xbt_str_split_str(key, sep);

    src_id = strtol(xbt_dynar_get_as(keys, 0, char *), &end, 16);
    dst_id = strtol(xbt_dynar_get_as(keys, 1, char *), &end, 16);
    xbt_dynar_free(&keys);

    DEBUG4("Handle %d %d (from %d hosts): %ld links",
        src_id,dst_id,routing->generic_routing.host_count,xbt_dynar_length(links));
    xbt_dynar_foreach(links, cpt, link_name) {
      void* link = xbt_dict_get_or_null(surf_network_model->resource_set, link_name);
      if (link)
        xbt_dynar_push(ROUTE_FULL(src_id,dst_id),&link);
      else
        THROW1(mismatch_error,0,"Link %s not found", link_name);
    }
  }

  /* Add the loopback if needed */
  for (i = 0; i < host_count; i++)
    if (!xbt_dynar_length(ROUTE_FULL(i, i)))
      xbt_dynar_push(ROUTE_FULL(i,i),&routing->loopback);

  /* Shrink the dynar routes (save unused slots) */
  for (i=0;i<host_count;i++)
    for (j=0;j<host_count;j++)
      xbt_dynar_shrink(ROUTE_FULL(i,j),0);
}

/*
 * Business methods
 */
static xbt_dynar_t routing_full_get_route(int src,int dst) {
  return ROUTE_FULL(src,dst);
}

static void routing_full_finalize(void) {
  routing_full_t routing = (routing_full_t)used_routing;
  int i,j;

  if (routing) {
    for (i = 0; i < used_routing->host_count; i++)
      for (j = 0; j < used_routing->host_count; j++)
        xbt_dynar_free(&ROUTE_FULL(i, j));
    free(routing->routing_table);
    xbt_dict_free(&used_routing->host_id);
    free(routing);
    routing=NULL;
  }
}

routing_t routing_model_full_create(size_t size_of_link,void *loopback) {
  /* initialize our structure */
  routing_full_t res = xbt_new0(s_routing_full_t,1);
  res->generic_routing.name = "Full";
  res->generic_routing.host_count = 0;
  res->generic_routing.get_route = routing_full_get_route;
  res->generic_routing.finalize = routing_full_finalize;
  res->size_of_link = size_of_link;
  res->loopback = loopback;

  /* Set it in position */
  used_routing = (routing_t) res;

  /* Setup the parsing callbacks we need */
  used_routing->host_id = xbt_dict_new();
  surfxml_add_callback(STag_surfxml_host_cb_list, &routing_full_parse_Shost);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &routing_full_parse_end);
  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &routing_full_parse_Sroute_set_endpoints);
  surfxml_add_callback(ETag_surfxml_route_cb_list, &routing_full_parse_Eroute);

  return used_routing;
}
