/*
 * surf.c
 *
 *  Created on: Nov 27, 2009
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#include "instr/private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(tracing_surf,tracing,"Tracing Surf");

#define VARIABLE_SEPARATOR '#'

static xbt_dict_t hosts_id;
static xbt_dict_t created_links;
static xbt_dict_t link_bandwidth;
static xbt_dict_t link_latency;
//static xbt_dict_t platform_variable_last_value; /* to control the amount of add/sub variables events*/
//static xbt_dict_t platform_variable_last_time; /* to control the amount of add/sub variables events*/

static xbt_dict_t last_platform_variables; /* to control the amount of add/sub variables events:
   dict with key {RESOURCE_NAME}#Time or {RESOURCE_NAME}#Value of dict with variables types == string */

static xbt_dict_t platform_variables; /* host or link name -> array of categories */

static xbt_dict_t resource_variables; /* (host|link)#variable -> value */

/* to trace gtnets */
static xbt_dict_t gtnets_src; /* %p (action) -> %s */
static xbt_dict_t gtnets_dst; /* %p (action) -> %s */

void __TRACE_surf_init (void)
{
  hosts_id = xbt_dict_new();
  created_links = xbt_dict_new();
  link_bandwidth = xbt_dict_new();
  link_latency = xbt_dict_new();
  platform_variables = xbt_dict_new();

  //platform_variable_last_value = xbt_dict_new();
  //platform_variable_last_time = xbt_dict_new();

  last_platform_variables =  xbt_dict_new();

  resource_variables = xbt_dict_new ();

  gtnets_src = xbt_dict_new ();
  gtnets_dst = xbt_dict_new ();
}

static char *strsplit (char *input, int field, char del) //caller should free the returned string
{
  int length = strlen(input), i;
  int s = 0, e = length+1;
  int current_field = 0;
  for (i = 0; i < length; i++){
     if (input[i] == del){
       if (current_field == field){
         e = i-1;
         break;
       }else{
         s = i+1;
         current_field++;
       }
     }
  }
  //copy string from s to e (with length equal to e-s) and return
  char *ret = malloc ((e-s+2)*sizeof(char));
  strncpy (ret, input+s, e-s+1);
  ret[e-s+1] = '\0';
  return ret;
}

void __TRACE_surf_finalize (void)
{
  if (!IS_TRACING_PLATFORM) return;
  if (!xbt_dict_length(last_platform_variables)){
    return;
  }else{
    xbt_dict_cursor_t cursor = NULL;
    unsigned int cursor_ar = 0;
    char *key, *value, *res;
    char resource[200];

    /* get all resources from last_platform_variables */
    xbt_dynar_t resources = xbt_dynar_new(sizeof(char)*200, xbt_free);
    xbt_dict_foreach(last_platform_variables, cursor, key, value) {
      res = strsplit (key, 0, VARIABLE_SEPARATOR);
      char *aux = strsplit (key, 1, VARIABLE_SEPARATOR);
      if (strcmp (aux, "Time") == 0){ //only need to add one of three
        xbt_dynar_push (resources, xbt_strdup(res));
      }
      free (aux);
      free (res);
    }

    /* iterate through resources array */
    xbt_dynar_foreach (resources, cursor_ar, resource) {
      char timekey[100], valuekey[100], variablekey[100];
      snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
      snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
      snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

      char *time = xbt_dict_get_or_null (last_platform_variables, timekey);
      if (!time) continue;
      char *value = xbt_dict_get (last_platform_variables, valuekey);
      char *variable = xbt_dict_get (last_platform_variables, variablekey);
      pajeSubVariable (atof(time), variable, resource, value);

      //TODO: should remove, but it is causing sigabort
      //xbt_dict_remove (last_platform_variables, timekey);
      //xbt_dict_remove (last_platform_variables, valuekey);
      //xbt_dict_remove (last_platform_variables, variablekey);
    }
  }
}

void __TRACE_surf_check_variable_set_to_zero (double now, const char *variable, const char *resource)
{
  /* check if we have to set it to 0 */
  if (!xbt_dict_get_or_null (platform_variables, resource)){
    xbt_dynar_t array = xbt_dynar_new(100*sizeof(char), xbt_free);
    xbt_dynar_push (array, xbt_strdup(variable));
    if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    xbt_dict_set (platform_variables, resource, array, xbt_dynar_free_voidp);
  }else{
    xbt_dynar_t array = xbt_dict_get (platform_variables, resource);
    unsigned int i;
    char cat[100];
    int flag = 0;
    xbt_dynar_foreach (array, i, cat) {
      if (strcmp(variable, cat)==0){
        flag = 1;
      }
    }
    if (flag==0){
      xbt_dynar_push (array, strdup(variable));
      if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    }
  }
  /* end of check */
}

void __TRACE_surf_update_action_state_resource (double now, double delta, const char *variable, const char *resource, double value)
{
  if (!IS_TRACING_PLATFORM) return;

  char valuestr[100];
  snprintf (valuestr, 100, "%f", value);

  /*
  //fprintf (stderr, "resource = %s variable = %s (%f -> %f) value = %s\n", resource, variable, now, now+delta, valuestr);
  if (1){
    __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
    if (IS_TRACING_PLATFORM) pajeAddVariable (now, variable, resource, valuestr);
    if (IS_TRACING_PLATFORM) pajeSubVariable (now+delta, variable, resource, valuestr);
    return;
  }
  */

  /*
   * The following code replaces the code above with the objective
   * to decrease the size of file because of unnecessary add/sub on
   * variables. It should be re-checked before put in production.
   */

  char nowstr[100], nowdeltastr[100];
  snprintf (nowstr, 100, "%.15f", now);
  snprintf (nowdeltastr, 100, "%.15f", now+delta);

  char timekey[100], valuekey[100], variablekey[100];
  snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
  snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
  snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

  char *lastvariable = xbt_dict_get_or_null (last_platform_variables, variablekey);
  if (lastvariable == NULL){
    __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
    pajeAddVariable (now, variable, resource, valuestr);
    xbt_dict_set (last_platform_variables, xbt_strdup (timekey), xbt_strdup (nowdeltastr), xbt_free);
    xbt_dict_set (last_platform_variables, xbt_strdup (valuekey), xbt_strdup (valuestr), xbt_free);
    xbt_dict_set (last_platform_variables, xbt_strdup (variablekey), xbt_strdup (variable), xbt_free);
  }else{
    char *lasttime = xbt_dict_get_or_null (last_platform_variables, timekey);
    char *lastvalue = xbt_dict_get_or_null (last_platform_variables, valuekey);

    /* check if it is the same variable */
    if (strcmp(lastvariable, variable) == 0){ /* same variable */
      /* check if lasttime equals now */
      if (atof(lasttime) == now){ /* lastime == now */
        /* check if lastvalue equals valuestr */
        if (atof(lastvalue) == value){ /* lastvalue == value (good, just advance time) */
          xbt_dict_set (last_platform_variables, xbt_strdup(timekey), xbt_strdup(nowdeltastr), xbt_free);
        }else{ /* value has changed */
          /* value has changed, subtract previous value, add new one */
          pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
          pajeAddVariable (atof(nowstr), variable, resource, valuestr);
          xbt_dict_set (last_platform_variables, xbt_strdup(timekey), xbt_strdup(nowdeltastr), xbt_free);
          xbt_dict_set (last_platform_variables, xbt_strdup(valuekey), xbt_strdup(valuestr), xbt_free);
        }
      }else{ /* lasttime != now */
        /* the last time is different from new starting time, subtract to lasttime and add from nowstr */
        pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
        pajeAddVariable (atof(nowstr), variable, resource, valuestr);
        xbt_dict_set (last_platform_variables, xbt_strdup(timekey), xbt_strdup(nowdeltastr), xbt_free);
        xbt_dict_set (last_platform_variables, xbt_strdup(valuekey), xbt_strdup(valuestr), xbt_free);
      }
    }else{ /* variable has changed */
      pajeSubVariable (atof(lasttime), lastvariable, resource, lastvalue);
      __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
      pajeAddVariable (now, variable, resource, valuestr);
      xbt_dict_set (last_platform_variables, xbt_strdup (timekey), xbt_strdup (nowdeltastr), xbt_free);
      xbt_dict_set (last_platform_variables, xbt_strdup (valuekey), xbt_strdup (valuestr), xbt_free);
      xbt_dict_set (last_platform_variables, xbt_strdup (variablekey), xbt_strdup (variable), xbt_free);
    }
  }
  return;
}

void __TRACE_surf_set_resource_variable (double date, const char *variable, const char *resource, double value)
{
  if (!IS_TRACING) return;
  char aux[100], key[100];
  snprintf (aux, 100, "%f", value);
  snprintf (key, 100, "%s%c%s", resource, VARIABLE_SEPARATOR, variable);

  char *last_value = xbt_dict_get_or_null(resource_variables, key);
  if (last_value){
    if (atof(last_value) == value){
      return;
    }
  }
  if (IS_TRACING_PLATFORM) pajeSetVariable (date, variable, resource, aux);
  xbt_dict_set (resource_variables, xbt_strdup(key), xbt_strdup(aux), xbt_free);
}

void TRACE_surf_update_action_state (void *surf_action, smx_action_t smx_action,
    double value, const char *stateValue, double now, double delta)
{
}

void TRACE_surf_update_action_state_net_resource (const char *name, smx_action_t smx_action, double value, double now, double delta)
{
  if (!IS_TRACING || !IS_TRACED(smx_action)) return;

  if (strcmp (name, "__loopback__")==0 ||
      strcmp (name, "loopback")==0){ //ignore loopback updates
    return;
  }

  if (value == 0) return;

  if (!xbt_dict_get_or_null (created_links, name)){
    TRACE_surf_missing_link ();
    return;
  }

  char type[100];
  snprintf (type, 100, "b%s", smx_action->category);
  __TRACE_surf_update_action_state_resource (now, delta, type, name, value);
  return;
}

void TRACE_surf_update_action_state_cpu_resource (const char *name, smx_action_t smx_action, double value, double now, double delta)
{
  if (!IS_TRACING || !IS_TRACED(smx_action)) return;

  if (value==0){
    return;
  }

  char type[100];
  snprintf (type, 100, "p%s", smx_action->category);
  __TRACE_surf_update_action_state_resource (now, delta, type, name, value);
  return;
}

void TRACE_surf_net_link_new (char *name, double bw, double lat)
{
  if (!IS_TRACING) return;
  //if (IS_TRACING_PLATFORM) pajeCreateContainerWithBandwidthLatency (SIMIX_get_clock(), name, "LINK", "platform", name, bw, lat);
  //save bw and lat information for this link
  double *bw_ptr, *lat_ptr;
  bw_ptr = xbt_new (double, 1);
  lat_ptr = xbt_new (double, 1);
  *bw_ptr = bw;
  *lat_ptr = lat;
  xbt_dict_set (link_bandwidth, xbt_strdup(name), bw_ptr, xbt_free);
  xbt_dict_set (link_latency, xbt_strdup(name), lat_ptr, xbt_free);
}

void TRACE_surf_cpu_new (char *name, double power)
{
  if (!IS_TRACING) return;
  if (IS_TRACING_PLATFORM) pajeCreateContainer (SIMIX_get_clock(), name, "HOST", "platform", name);
  __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "power", name, power);
}

void TRACE_surf_routing_full_parse_end (char *link_name, int src, int dst)
{
  if (!IS_TRACING) return;
  char srcidstr[100], dstidstr[100];
  snprintf (srcidstr, 100, "%d", src);
  snprintf (dstidstr, 100, "%d", dst);
  char *srcname = xbt_dict_get (hosts_id, srcidstr);
  char *dstname = xbt_dict_get (hosts_id, dstidstr);

  char key[100];
  snprintf (key, 100, "l%d-%d", src, dst);

  if (strcmp (link_name, "__loopback__")==0 ||
      strcmp (link_name, "loopback")==0){ //ignore loopback updates
    return;
  }

  if (!xbt_dict_get_or_null (created_links, link_name)){
    //if (IS_TRACING_PLATFORM) pajeStartLink (SIMIX_get_clock(), "edge", "platform", "route", srcname, key);
    //if (IS_TRACING_PLATFORM) pajeEndLink (SIMIX_get_clock()+0.1, "edge", "platform", "route", dstname, key);
    double *bw = xbt_dict_get (link_bandwidth, link_name);
    double *lat = xbt_dict_get (link_latency, link_name);
    if (IS_TRACING_PLATFORM) pajeCreateContainerWithBandwidthLatencySrcDst (SIMIX_get_clock(), link_name, "LINK", "platform", link_name, *bw, *lat, srcname, dstname);
    __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "bandwidth", link_name, *bw);
    __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "latency", link_name, *lat);
    xbt_dict_set (created_links, xbt_strdup(link_name), xbt_strdup ("1"), xbt_free);
  }
}

void TRACE_surf_cpu_set_power (double date, char *resource, double power)
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

void TRACE_surf_define_host_id (const char *name, int host_id)
{
  if (!IS_TRACING) return;
  char strid[100];
  snprintf (strid, 100, "%d", host_id);
  xbt_dict_set (hosts_id, strdup(name), strdup(strid), free);
  xbt_dict_set (hosts_id, strdup(strid), strdup(name), free);
}

/* to trace gtnets */
void TRACE_surf_gtnets_communicate (void *action, int src, int dst)
{
  if (!IS_TRACING) return;
  char key[100], aux[100];
  snprintf (key, 100, "%p", action);

  snprintf (aux, 100, "%d", src);
  xbt_dict_set (gtnets_src, xbt_strdup(key), xbt_strdup(aux), xbt_free);
  snprintf (aux, 100, "%d", dst);
  xbt_dict_set (gtnets_dst, xbt_strdup(key), xbt_strdup(aux), xbt_free);
}

int TRACE_surf_gtnets_get_src (void *action)
{
  if (!IS_TRACING) return -1;
  char key[100];
  snprintf (key, 100, "%p", action);

  char *aux = xbt_dict_get_or_null (gtnets_src, key);
  if (aux){
	return atoi(aux);
  }else{
    return -1;
  }
}

int TRACE_surf_gtnets_get_dst (void *action)
{
  if (!IS_TRACING) return -1;
  char key[100];
  snprintf (key, 100, "%p", action);

  char *aux = xbt_dict_get_or_null (gtnets_dst, key);
  if (aux){
	return atoi(aux);
  }else{
    return -1;
  }
}

void TRACE_surf_gtnets_destroy (void *action)
{
  if (!IS_TRACING) return;
  char key[100];
  snprintf (key, 100, "%p", action);
  xbt_dict_remove (gtnets_src, key);
  xbt_dict_remove (gtnets_dst, key);
}

void TRACE_surf_missing_link (void)
{
  CRITICAL0("The trace cannot be done because "
		 "the platform you are using contains "
		 "routes with more than one link.");
  THROW0(tracing_error, TRACE_ERROR_COMPLEX_ROUTES, "Tracing failed");
}

#endif
