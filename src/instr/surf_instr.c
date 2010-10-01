/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(tracing_surf,tracing,"Tracing Surf");

#define VARIABLE_SEPARATOR '#'

static xbt_dict_t created_links;
static xbt_dict_t host_containers;
static xbt_dict_t platform_variables; /* host or link name -> array of categories */
static xbt_dict_t resource_variables; /* (host|link)#variable -> value */

/* to trace gtnets */
static xbt_dict_t gtnets_src; /* %p (action) -> %s */
static xbt_dict_t gtnets_dst; /* %p (action) -> %s */

void __TRACE_surf_init (void)
{
  created_links = xbt_dict_new();
  platform_variables = xbt_dict_new();
  host_containers = xbt_dict_new();
  resource_variables = xbt_dict_new ();
  gtnets_src = xbt_dict_new ();
  gtnets_dst = xbt_dict_new ();
  __TRACE_surf_resource_utilization_initialize();
}

void __TRACE_surf_finalize (void)
{
  __TRACE_surf_resource_utilization_finalize();
}

void __TRACE_surf_check_variable_set_to_zero (double now, const char *variable, const char *resource)
{
  /* check if we have to set it to 0 */
  if (!xbt_dict_get_or_null (platform_variables, resource)){
    xbt_dynar_t array = xbt_dynar_new(sizeof(char*), xbt_free);
    char *var_cpy = xbt_strdup(variable);
    xbt_dynar_push (array, &var_cpy);
    if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    xbt_dict_set (platform_variables, resource, array, xbt_dynar_free_voidp);
  }else{
    xbt_dynar_t array = xbt_dict_get (platform_variables, resource);
    unsigned int i;
    char* cat;
    int flag = 0;
    xbt_dynar_foreach (array, i, cat) {
      if (strcmp(variable, cat)==0){
        flag = 1;
      }
    }
    if (flag==0){
      char *var_cpy = xbt_strdup(variable);
      xbt_dynar_push (array, &var_cpy);
      if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    }
  }
  /* end of check */
}

void __TRACE_surf_set_resource_variable (double date, const char *variable, const char *resource, double value)
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
void TRACE_surf_link_declaration (char *name, double bw, double lat)
{
  if (!IS_TRACING) return;

  if (strcmp (name, "__loopback__")==0 ||
      strcmp (name, "loopback")==0){ //ignore loopback updates
    return;
  }

  pajeCreateContainer (SIMIX_get_clock(), name, "LINK", "platform", name);
  xbt_dict_set (created_links, name, xbt_strdup ("1"), xbt_free);
  TRACE_surf_link_set_bandwidth (SIMIX_get_clock(), name, bw);
  TRACE_surf_link_set_latency (SIMIX_get_clock(), name, lat);
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
void TRACE_surf_host_declaration (char *name, double power)
{
  if (!IS_TRACING) return;
  pajeCreateContainer (SIMIX_get_clock(), name, "HOST", "platform", name);
  xbt_dict_set (host_containers, name, xbt_strdup("1"), xbt_free);
  TRACE_surf_host_set_power (SIMIX_get_clock(), name, power);
}

void TRACE_surf_host_set_power (double date, char *resource, double power)
{
  __TRACE_surf_set_resource_variable (date, "power", resource, power);
}

void TRACE_surf_link_set_bandwidth (double date, char *resource, double bandwidth)
{
  __TRACE_surf_set_resource_variable (date, "bandwidth", resource, bandwidth);
}

void TRACE_surf_link_set_latency (double date, char *resource, double latency)
{
  __TRACE_surf_set_resource_variable (date, "latency", resource, latency);
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

void TRACE_surf_link_missing (void)
{
  CRITICAL0("The trace cannot be done because "
		 "the platform you are using contains "
		 "routes with more than one link.");
  THROW0(tracing_error, TRACE_ERROR_COMPLEX_ROUTES, "Tracing failed");
}

void TRACE_msg_clean (void)
{
  char *key, *value;
  xbt_dict_cursor_t cursor = NULL;
  __TRACE_surf_finalize();

  /* get all host from host_containers */
  xbt_dict_foreach(host_containers, cursor, key, value) {
    pajeDestroyContainer (MSG_get_clock(), "HOST", key);
  }
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

#endif
