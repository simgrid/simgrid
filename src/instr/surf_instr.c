/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(tracing_surf,tracing,"Tracing Surf");

#define VARIABLE_SEPARATOR '#'

static xbt_dict_t hosts_id;
static xbt_dict_t created_links;
static xbt_dict_t link_bandwidth; //name(char*) -> bandwidth(double)
static xbt_dict_t link_latency;   //name(char*) -> latency(double)
static xbt_dict_t host_containers;

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
  host_containers = xbt_dict_new();
  last_platform_variables =  xbt_dict_new();
  resource_variables = xbt_dict_new ();
  gtnets_src = xbt_dict_new ();
  gtnets_dst = xbt_dict_new ();
}

static char *strsplit (char *input, int field, char del) //caller should free the returned string
{
	char *ret = NULL;
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
  ret = malloc ((e-s+2)*sizeof(char));
  strncpy (ret, input+s, e-s+1);
  ret[e-s+1] = '\0';
  return ret;
}

void __TRACE_surf_finalize (void)
{
	xbt_dict_cursor_t cursor = NULL;
	unsigned int cursor_ar = 0;
    char *key, *value, *res;
    char *resource;
    char *aux = NULL;
    char *var_cpy = NULL;
    xbt_dynar_t resources = NULL;
  if (!IS_TRACING_PLATFORM) return;
  if (!xbt_dict_length(last_platform_variables)){
    return;
  }else{
    /* get all resources from last_platform_variables */
    resources = xbt_dynar_new(sizeof(char*), xbt_free);
    xbt_dict_foreach(last_platform_variables, cursor, key, value) {
      res = strsplit (key, 0, VARIABLE_SEPARATOR);
      aux = strsplit (key, 1, VARIABLE_SEPARATOR);
      if (strcmp (aux, "Time") == 0){ //only need to add one of three
        var_cpy = xbt_strdup (res);
        xbt_dynar_push (resources, &var_cpy);
      }
      free (aux);
      free (res);
    }

    /* iterate through resources array */
    xbt_dynar_foreach (resources, cursor_ar, resource) {
      char timekey[100], valuekey[100], variablekey[100];
      char *time = NULL;
      char *value = NULL;
      char *variable = NULL;
      snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
      snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
      snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

      time = xbt_dict_get_or_null (last_platform_variables, timekey);
      if (!time) continue;
      value = xbt_dict_get (last_platform_variables, valuekey);
      variable = xbt_dict_get (last_platform_variables, variablekey);
      pajeSubVariable (atof(time), variable, resource, value);

      xbt_dict_remove (last_platform_variables, timekey);
      xbt_dict_remove (last_platform_variables, valuekey);
      xbt_dict_remove (last_platform_variables, variablekey);
    }
  }
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

void __TRACE_surf_update_action_state_resource (double now, double delta, const char *variable, const char *resource, double value)
{
	char valuestr[100];
	char nowstr[100], nowdeltastr[100];
	char timekey[100], valuekey[100], variablekey[100];
	char *lastvariable = NULL;
	char *lasttime = NULL;
    char *lastvalue = NULL;
    char *nowdeltastr_cpy = NULL;
    char *valuestr_cpy = NULL;
    char *variable_cpy = NULL;

  if (!IS_TRACING_PLATFORM) return;

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

  snprintf (nowstr, 100, "%.15f", now);
  snprintf (nowdeltastr, 100, "%.15f", now+delta);

  snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
  snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
  snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

  lastvariable = xbt_dict_get_or_null (last_platform_variables, variablekey);
  if (lastvariable == NULL){
    __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
    pajeAddVariable (now, variable, resource, valuestr);
    nowdeltastr_cpy = xbt_strdup (nowdeltastr);
    valuestr_cpy = xbt_strdup (valuestr);
    variable_cpy = xbt_strdup (variable);
    xbt_dict_set (last_platform_variables, timekey, nowdeltastr_cpy, xbt_free);
    xbt_dict_set (last_platform_variables, valuekey, valuestr_cpy, xbt_free);
    xbt_dict_set (last_platform_variables, variablekey, variable_cpy, xbt_free);
  }else{
    lasttime = xbt_dict_get_or_null (last_platform_variables, timekey);
    lastvalue = xbt_dict_get_or_null (last_platform_variables, valuekey);

    /* check if it is the same variable */
    if (strcmp(lastvariable, variable) == 0){ /* same variable */
      /* check if lasttime equals now */
      if (atof(lasttime) == now){ /* lastime == now */
        /* check if lastvalue equals valuestr */
        if (atof(lastvalue) == value){ /* lastvalue == value (good, just advance time) */
          char *nowdeltastr_cpy = xbt_strdup (nowdeltastr);
          xbt_dict_set (last_platform_variables, timekey, nowdeltastr_cpy, xbt_free);
        }else{ /* value has changed */
          /* value has changed, subtract previous value, add new one */
          pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
          pajeAddVariable (atof(nowstr), variable, resource, valuestr);
          nowdeltastr_cpy = xbt_strdup (nowdeltastr);
          valuestr_cpy = xbt_strdup (valuestr);
          xbt_dict_set (last_platform_variables, timekey, nowdeltastr_cpy, xbt_free);
          xbt_dict_set (last_platform_variables, valuekey, valuestr_cpy, xbt_free);
        }
      }else{ /* lasttime != now */
        /* the last time is different from new starting time, subtract to lasttime and add from nowstr */
        pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
        pajeAddVariable (atof(nowstr), variable, resource, valuestr);
        nowdeltastr_cpy = xbt_strdup (nowdeltastr);
        valuestr_cpy = xbt_strdup (valuestr);
        xbt_dict_set (last_platform_variables, timekey, nowdeltastr_cpy, xbt_free);
        xbt_dict_set (last_platform_variables, valuekey, valuestr_cpy, xbt_free);
      }
    }else{ /* variable has changed */
      pajeSubVariable (atof(lasttime), lastvariable, resource, lastvalue);
      __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
      pajeAddVariable (now, variable, resource, valuestr);
      nowdeltastr_cpy = xbt_strdup (nowdeltastr);
      valuestr_cpy = xbt_strdup (valuestr);
      variable_cpy = xbt_strdup (variable);
      xbt_dict_set (last_platform_variables, timekey, nowdeltastr_cpy, xbt_free);
      xbt_dict_set (last_platform_variables, valuekey, valuestr_cpy, xbt_free);
      xbt_dict_set (last_platform_variables, variablekey, variable_cpy, xbt_free);
    }
  }
  return;
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

void TRACE_surf_link_set_utilization (const char *name, smx_action_t smx_action, double value, double now, double delta)
{
	char type[100];
  if (!IS_TRACING || !IS_TRACED(smx_action)) return;

  if (strcmp (name, "__loopback__")==0 ||
      strcmp (name, "loopback")==0){ //ignore loopback updates
    return;
  }

  if (value == 0) return;

  if (!xbt_dict_get_or_null (created_links, name)){
    TRACE_surf_link_missing ();
    return;
  }

  snprintf (type, 100, "b%s", smx_action->category);
  __TRACE_surf_update_action_state_resource (now, delta, type, name, value);
  return;
}

void TRACE_surf_host_set_utilization (const char *name, smx_action_t smx_action, double value, double now, double delta)
{
	char type[100];
  if (!IS_TRACING || !IS_TRACED(smx_action)) return;

  if (value==0){
    return;
  }

  snprintf (type, 100, "p%s", smx_action->category);
  __TRACE_surf_update_action_state_resource (now, delta, type, name, value);
  return;
}

/*
 * TRACE_surf_link_declaration (name, bandwidth, latency): this function
 * saves the bandwidth and latency of a link identified by name. This
 * information is used in the future to create the link container in the trace.
 *
 * caller: net_link_new (from each network model)
 * main: save bandwidth and latency information
 * return: void
 */
void TRACE_surf_link_declaration (char *name, double bw, double lat)
{
  if (!IS_TRACING) return;

  double *bw_ptr = xbt_malloc(sizeof(double));
  double *lat_ptr = xbt_malloc(sizeof(double));
  *bw_ptr = bw;
  *lat_ptr = lat;
  xbt_dict_set (link_bandwidth, name, bw_ptr, xbt_free);
  xbt_dict_set (link_latency, name, lat_ptr, xbt_free);
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
  if (IS_TRACING_PLATFORM){
    __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "power", name, power);
  }
}

/*
 * TRACE_surf_link_save_endpoints (link_name, src, dst): this function
 * creates the container of a link identified by link_name. It gets
 * bandwidth and latency information from the dictionaries previously
 * filled. The end points are obtained from the hosts_id dictionary.
 *
 * caller: end of parsing
 * main: create LINK containers, set initial bandwidth and latency values
 * return: void
 */
void TRACE_surf_link_save_endpoints (char *link_name, int src, int dst)
{
	char srcidstr[100], dstidstr[100];
	char key[100];

  if (!IS_TRACING) return;
  snprintf (srcidstr, 100, "%d", src);
  snprintf (dstidstr, 100, "%d", dst);
  char *srcname = xbt_dict_get (hosts_id, srcidstr);
  char *dstname = xbt_dict_get (hosts_id, dstidstr);
  snprintf (key, 100, "l%d-%d", src, dst);

  if (strcmp (link_name, "__loopback__")==0 ||
      strcmp (link_name, "loopback")==0){ //ignore loopback updates
    return;
  }

  if (!xbt_dict_get_or_null (created_links, link_name)){
    char *bw = xbt_dict_get (link_bandwidth, link_name);
    char *lat = xbt_dict_get (link_latency, link_name);
    pajeCreateContainerWithBandwidthLatencySrcDst (SIMIX_get_clock(), link_name, "LINK", "platform", link_name, *bw, *lat, srcname, dstname);
    if (IS_TRACING_PLATFORM) __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "bandwidth", link_name, *bw);
    if (IS_TRACING_PLATFORM) __TRACE_surf_set_resource_variable (SIMIX_get_clock(), "latency", link_name, *lat);
    xbt_dict_set (created_links, link_name, xbt_strdup ("1"), xbt_free);
  }
}

/*
 * TRACE_surf_host_define_id (name, host_id): This function relates
 * the name of the host with its id, as generated by the called and
 * passed as second parameter.
 *
 * caller: host parser, router parser
 * main: save host_id
 * return: void
 */
void TRACE_surf_host_define_id (const char *name, int host_id)
{
  char strid[100];
  if (!IS_TRACING) return;
  snprintf (strid, 100, "%d", host_id);
  xbt_dict_set (hosts_id, name, strdup(strid), free);
  xbt_dict_set (hosts_id, strid, strdup(name), free);
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
