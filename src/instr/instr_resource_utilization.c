/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_resource, instr, "tracing (un)-categorized resource utilization");

//to check if variables were previously set to 0, otherwise paje won't simulate them
static xbt_dict_t platform_variables;   /* host or link name -> array of categories */

//used by all methods
static void __TRACE_surf_check_variable_set_to_zero(double now,
                                                    const char *variable,
                                                    const char *resource)
{
  /* check if we have to set it to 0 */
  if (!xbt_dict_get_or_null(platform_variables, resource)) {
    xbt_dynar_t array = xbt_dynar_new(sizeof(char *), xbt_free);
    char *var_cpy = xbt_strdup(variable);
    xbt_dynar_push(array, &var_cpy);
    container_t container = getContainerByName (resource);
    type_t type = getVariableType (variable, NULL, container->type);
    new_pajeSetVariable (now, container, type, 0);
    xbt_dict_set(platform_variables, resource, array, NULL);
  } else {
    xbt_dynar_t array = xbt_dict_get(platform_variables, resource);
    unsigned int i;
    char *cat;
    int flag = 0;
    xbt_dynar_foreach(array, i, cat) {
      if (strcmp(variable, cat) == 0) {
        flag = 1;
      }
    }
    if (flag == 0) {
      char *var_cpy = xbt_strdup(variable);
      xbt_dynar_push(array, &var_cpy);
      if (TRACE_categorized ()){
        container_t container = getContainerByName (resource);
        type_t type = getVariableType (variable, NULL, container->type);
        new_pajeSetVariable (now, container, type, 0);
      }
    }
  }
  /* end of check */
}



/*
static void __TRACE_A_event(smx_action_t action, double now, double delta,
                            const char *variable, const char *resource,
                            double value)
{
  char valuestr[100];
  snprintf(valuestr, 100, "%f", value);

  __TRACE_surf_check_variable_set_to_zero(now, variable, resource);
  container_t container = getContainerByName (resource);
  type_t type = getVariableType (variable, NULL, container->type);
  new_pajeAddVariable(now, container, type, value);
  new_pajeSubVariable(now + delta, container, type, value);
}
*/

static void instr_event (double now, double delta, type_t variable, container_t resource, double value)
{
  __TRACE_surf_check_variable_set_to_zero(now, variable->name, resource->name);
  new_pajeAddVariable(now, resource, variable, value);
  new_pajeSubVariable(now + delta, resource, variable, value);
}

/*
 * TRACE_surf_link_set_utilization: entry point from SimGrid
 */
void TRACE_surf_link_set_utilization(const char *resource, smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta)
{
  //only trace link utilization if link is known by tracing mechanism
  if (!knownContainerWithName(resource))
    return;
  if (!value)
    return;

  //trace uncategorized link utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT LINK [%f - %f] %s bandwidth_used %f", now, now+delta, resource, value);
    container_t container = getContainerByName (resource);
    type_t type = getVariableType("bandwidth_used", NULL, container->type);
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (!surf_action->category)
      return;
    //variable of this category starts by 'b', because we have a link here
    char category_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (category_type, INSTR_DEFAULT_STR_SIZE, "b%s", surf_action->category);
    XBT_DEBUG("CAT LINK [%f - %f] %s %s %f", now, now+delta, resource, category_type, value);
    container_t container = getContainerByName (resource);
    type_t type = getVariableType(category_type, NULL, container->type);
    instr_event (now, delta, type, container, value);
  }
  return;
}

/*
 * TRACE_surf_host_set_utilization: entry point from SimGrid
 */
void TRACE_surf_host_set_utilization(const char *resource,
                                     smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta)
{
  //only trace host utilization if host is known by tracing mechanism
  if (!knownContainerWithName(resource))
    return;
  if (!value)
    return;

  //trace uncategorized host utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT HOST [%f - %f] %s power_used %f", now, now+delta, resource, value);
    container_t container = getContainerByName (resource);
    type_t type = getVariableType("power_used", NULL, container->type);
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (!surf_action->category)
      return;
    //variable of this category starts by 'p', because we have a host here
    char category_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (category_type, INSTR_DEFAULT_STR_SIZE, "p%s", surf_action->category);
    XBT_DEBUG("CAT HOST [%f - %f] %s %s %f", now, now+delta, resource, category_type, value);
    container_t container = getContainerByName (resource);
    type_t type = getVariableType(category_type, NULL, container->type);
    instr_event (now, delta, type, container, value);
  }
  return;
}

void TRACE_surf_resource_utilization_alloc()
{
  platform_variables = xbt_dict_new_homogeneous(xbt_dynar_free_voidp);
}

void TRACE_surf_resource_utilization_release()
{
}
#endif /* HAVE_TRACING */
