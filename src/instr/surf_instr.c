/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"
#include "surf/surf_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(tracing_surf,tracing,"Tracing Surf");

#define VARIABLE_SEPARATOR '#'

static xbt_dict_t created_links;
static xbt_dict_t host_containers;
static xbt_dict_t resource_variables; /* (host|link)#variable -> value */

/* to trace gtnets */
static xbt_dict_t gtnets_src; /* %p (action) -> %s */
static xbt_dict_t gtnets_dst; /* %p (action) -> %s */

void TRACE_surf_alloc (void)
{
  created_links = xbt_dict_new();
  host_containers = xbt_dict_new();
  resource_variables = xbt_dict_new ();
  gtnets_src = xbt_dict_new ();
  gtnets_dst = xbt_dict_new ();

  TRACE_surf_resource_utilization_alloc();
}

void TRACE_surf_release (void)
{
  char *key, *value;
  xbt_dict_cursor_t cursor = NULL;
  TRACE_surf_resource_utilization_release();

  /* get all host from host_containers */
  xbt_dict_foreach(host_containers, cursor, key, value) {
    pajeDestroyContainer (MSG_get_clock(), "HOST", key);
  }
  xbt_dict_foreach(created_links, cursor, key, value) {
    pajeDestroyContainer (MSG_get_clock(), "LINK", key);
  }
}

static void TRACE_surf_set_resource_variable (double date, const char *variable, const char *resource, double value)
{
	char aux[100], key[100];
	char *last_value = NULL;
  if (!IS_TRACING) return;
  snprintf (aux, 100, "%f", value);
  snprintf (key, 100, "%s%c%s", resource, VARIABLE_SEPARATOR, variable);

  last_value = xbt_dict_get_or_null(resource_variables, key);
  if (last_value){
    if (atof(last_value) == value){
      return;
    }
  }
  if (IS_TRACING_PLATFORM) pajeSetVariable (date, variable, resource, aux);
  xbt_dict_set (resource_variables, xbt_strdup(key), xbt_strdup(aux), xbt_free);
}

/*
 * TRACE_surf_link_declaration (name, bandwidth, latency): this function
 * saves the bandwidth and latency of a link identified by name. This
 * information is used in the future to create the link container in the trace.
 *
 * caller: net_link_new (from each network model)
 * main: create LINK container, set initial bandwidth and latency
 * return: void
 */
void TRACE_surf_link_declaration (void *link, char *name, double bw, double lat)
{
  if (!IS_TRACING) return;

  //filter out loopback
  if (!strcmp (name, "loopback") || !strcmp (name, "__loopback__")) return;

  char alias[100];
  snprintf (alias, 100, "%p", link);
  pajeCreateContainer (SIMIX_get_clock(), alias, "LINK", "platform", name);
  xbt_dict_set (created_links, alias, xbt_strdup ("1"), xbt_free);
  TRACE_surf_link_set_bandwidth (SIMIX_get_clock(), link, bw);
  TRACE_surf_link_set_latency (SIMIX_get_clock(), link, lat);
}

/*
 * TRACE_surf_host_declaration (name, power): this function
 * saves the power of a host identified by name. This information
 * is used to create the host container in the trace.
 *
 * caller: cpu_new (from each cpu model) + router parser
 * main: create HOST containers, set initial power value
 * return: void
 */
void TRACE_surf_host_declaration (const char *name, double power)
{
  if (!IS_TRACING) return;
  pajeCreateContainer (SIMIX_get_clock(), name, "HOST", "platform", name);
  xbt_dict_set (host_containers, name, xbt_strdup("1"), xbt_free);
  TRACE_surf_host_set_power (SIMIX_get_clock(), name, power);
}

void TRACE_surf_host_set_power (double date, const char *resource, double power)
{
  TRACE_surf_set_resource_variable (date, "power", resource, power);
}

void TRACE_surf_link_set_bandwidth (double date, void *link, double bandwidth)
{
  if (!TRACE_surf_link_is_traced (link)) return;

  char resource[100];
  snprintf (resource, 100, "%p", link);
  TRACE_surf_set_resource_variable (date, "bandwidth", resource, bandwidth);
}

void TRACE_surf_link_set_latency (double date, void *link, double latency)
{
  if (!TRACE_surf_link_is_traced (link)) return;

  char resource[100];
  snprintf (resource, 100, "%p", link);
  TRACE_surf_set_resource_variable (date, "latency", resource, latency);
}

/* to trace gtnets */
void TRACE_surf_gtnets_communicate (void *action, int src, int dst)
{
	char key[100], aux[100];
  if (!IS_TRACING) return;
  snprintf (key, 100, "%p", action);

  snprintf (aux, 100, "%d", src);
  xbt_dict_set (gtnets_src, key, xbt_strdup(aux), xbt_free);
  snprintf (aux, 100, "%d", dst);
  xbt_dict_set (gtnets_dst, key, xbt_strdup(aux), xbt_free);
}

int TRACE_surf_gtnets_get_src (void *action)
{
	char key[100];
	char *aux = NULL;
  if (!IS_TRACING) return -1;
  snprintf (key, 100, "%p", action);

  aux = xbt_dict_get_or_null (gtnets_src, key);
  if (aux){
	return atoi(aux);
  }else{
    return -1;
  }
}

int TRACE_surf_gtnets_get_dst (void *action)
{
	char key[100];
	char *aux = NULL;
  if (!IS_TRACING) return -1;
  snprintf (key, 100, "%p", action);

  aux = xbt_dict_get_or_null (gtnets_dst, key);
  if (aux){
	return atoi(aux);
  }else{
    return -1;
  }
}

void TRACE_surf_gtnets_destroy (void *action)
{
  char key[100];
  if (!IS_TRACING) return;
  snprintf (key, 100, "%p", action);
  xbt_dict_remove (gtnets_src, key);
  xbt_dict_remove (gtnets_dst, key);
}

void TRACE_surf_host_vivaldi_parse (char *host, double x, double y, double h)
{
	char valuestr[100];
  if (!IS_TRACING || !IS_TRACING_PLATFORM) return;

  snprintf (valuestr, 100, "%g", x);
  pajeSetVariable (0, "vivaldi_x", host, valuestr);
  snprintf (valuestr, 100, "%g", y);
  pajeSetVariable (0, "vivaldi_y", host, valuestr);
  snprintf (valuestr, 100, "%g", h);
  pajeSetVariable (0, "vivaldi_h", host, valuestr);
}

extern routing_global_t global_routing;
void TRACE_surf_save_onelink (void)
{
  if (!IS_TRACING) return;

  //get the onelinks from the parsed platform
  xbt_dynar_t onelink_routes = global_routing->get_onelink_routes();
  if (!onelink_routes) return;

  //save them in trace file
  onelink_t onelink;
  unsigned int iter;
  xbt_dynar_foreach(onelink_routes, iter, onelink) {
    char *src = onelink->src;
    char *dst = onelink->dst;
    void *link = onelink->link_ptr;

    if (TRACE_surf_link_is_traced (link)){
      char resource[100];
      snprintf (resource, 100, "%p", link);

      pajeNewEvent (0.1, "source", resource, src);
      pajeNewEvent (0.1, "destination", resource, dst);
    }
  }
}

int TRACE_surf_link_is_traced (void *link)
{
  char alias[100];
  snprintf (alias, 100, "%p", link);
  if (xbt_dict_get_or_null (created_links, alias)){
    return 1;
  }else{
    return 0;
  }
}

void TRACE_surf_action (surf_action_t surf_action, const char *category)
{
  if (!IS_TRACING_PLATFORM) return;
  if (!category){
    xbt_die ("invalid tracing category");
  }
  surf_action->category = xbt_new (char, strlen (category)+1);
  strncpy (surf_action->category, category, strlen(category)+1);
}
#endif
