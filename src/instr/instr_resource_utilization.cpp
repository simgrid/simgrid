/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_resource, instr, "tracing (un)-categorized resource utilization");

//to check if variables were previously set to 0, otherwise paje won't simulate them
static xbt_dict_t platform_variables;

//used by all methods
static void __TRACE_surf_check_variable_set_to_zero(double now, const char *variable, const char *resource)
{
  /* To trace resource utilization, we use pajeAddVariable and pajeSubVariable only.
   * The Paje simulator needs a pajeSetVariable in the first place so it knows the initial value of all variables for
   * subsequent adds/subs. If we don't do so, the first pajeAddVariable is added to a non-determined value within
   * the Paje simulator, causing analysis problems.
   */

  // create a key considering the resource and variable
  int n = strlen(variable)+strlen(resource)+1;
  char *key = (char*)xbt_malloc(n*sizeof(char));
  snprintf (key, n, "%s%s", resource, variable);

  // check if key exists: if it doesn't, set the variable to zero and mark this in the dict
  if (!xbt_dict_get_or_null(platform_variables, key)) {
    container_t container = PJ_container_get (resource);
    type_t type = PJ_type_get (variable, container->type);
    new_pajeSetVariable (now, container, type, 0);
    xbt_dict_set(platform_variables, key, (char*)"", NULL);
  }
  xbt_free(key);
}

static void instr_event (double now, double delta, type_t variable, container_t resource, double value)
{
  __TRACE_surf_check_variable_set_to_zero(now, variable->name, resource->name);
  new_pajeAddVariable(now, resource, variable, value);
  new_pajeSubVariable(now + delta, resource, variable, value);
}

/* TRACE_surf_link_set_utilization: entry point from SimGrid */
void TRACE_surf_link_set_utilization(const char *resource, const char *category, double value, double now, double delta)
{
  //only trace link utilization if link is known by tracing mechanism
  if (!PJ_container_get_or_null(resource))
    return;
  if (!value)
    return;

  //trace uncategorized link utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT LINK [%f - %f] %s bandwidth_used %f", now, now+delta, resource, value);
    container_t container = PJ_container_get (resource);
    type_t type = PJ_type_get ("bandwidth_used", container->type);
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (!category)
      return;
    //variable of this category starts by 'b', because we have a link here
    char category_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (category_type, INSTR_DEFAULT_STR_SIZE, "b%s", category);
    XBT_DEBUG("CAT LINK [%f - %f] %s %s %f", now, now+delta, resource, category_type, value);
    container_t container = PJ_container_get (resource);
    type_t type = PJ_type_get (category_type, container->type);
    instr_event (now, delta, type, container, value);
  }
  return;
}

/* TRACE_surf_host_set_utilization: entry point from SimGrid */
void TRACE_surf_host_set_utilization(const char *resource, const char *category, double value, double now, double delta)
{
  //only trace host utilization if host is known by tracing mechanism
  container_t container = PJ_container_get_or_null(resource);
  if (!container)
    return;
  if (!value)
    return;

  //trace uncategorized host utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT HOST [%f - %f] %s power_used %f", now, now+delta, resource, value);
    type_t type = PJ_type_get ("power_used", container->type);
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (!category)
      return;
    //variable of this category starts by 'p', because we have a host here
    char category_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (category_type, INSTR_DEFAULT_STR_SIZE, "p%s", category);
    XBT_DEBUG("CAT HOST [%f - %f] %s %s %f", now, now+delta, resource, category_type, value);
    type_t type = PJ_type_get (category_type, container->type);
    instr_event (now, delta, type, container, value);
  }
  return;
}

void TRACE_surf_resource_utilization_alloc()
{
  platform_variables = xbt_dict_new_homogeneous(NULL);
}

void TRACE_surf_resource_utilization_release()
{
  xbt_dict_free(&platform_variables);
}
