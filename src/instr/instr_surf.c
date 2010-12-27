/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "surf/surf_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_surf, instr, "Tracing Surf");

void TRACE_surf_alloc(void)
{
  TRACE_surf_resource_utilization_alloc();
}

void TRACE_surf_release(void)
{
  TRACE_surf_resource_utilization_release();
}

static void TRACE_surf_set_resource_variable(double date,
                                             const char *variable,
                                             const char *resource,
                                             double value)
{
  container_t container = getContainerByName(resource);
  type_t type = getVariableType(variable, NULL, container->type);
  new_pajeSetVariable(date, container, type, value);
}

void TRACE_surf_host_set_power(double date, const char *resource, double power)
{
  if (!TRACE_is_active())
    return;

  char *variable_type = getVariableTypeIdByName("power", getContainerByName(resource)->type);
  TRACE_surf_set_resource_variable(date, variable_type, resource, power);
}

void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth)
{
  if (!TRACE_is_active())
    return;

  char *variable_type = getVariableTypeIdByName("bandwidth", getContainerByName(resource)->type);
  TRACE_surf_set_resource_variable(date, variable_type, resource, bandwidth);
}

//FIXME: this function is not used (latency availability traces support exists in surf network models?)
void TRACE_surf_link_set_latency(double date, const char *resource, double latency)
{
  if (!TRACE_is_active())
    return;

  char *variable_type = getVariableTypeIdByName("latency", getContainerByName(resource)->type);
  TRACE_surf_set_resource_variable(date, variable_type, resource, latency);
}

/* to trace gtnets */
void TRACE_surf_gtnets_communicate(void *action, int src, int dst)
{
}

int TRACE_surf_gtnets_get_src(void *action)
{
  return -1;
}

int TRACE_surf_gtnets_get_dst(void *action)
{
  return -1;
}

void TRACE_surf_gtnets_destroy(void *action)
{
}

void TRACE_surf_action(surf_action_t surf_action, const char *category)
{
  if (!TRACE_is_active())
    return;
  if (!TRACE_categorized ())
    return;
  if (!category)
    return;

  surf_action->category = xbt_strdup(category);
}
#endif /* HAVE_TRACING */
