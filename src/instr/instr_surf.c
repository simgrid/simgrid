/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "surf/surf_private.h"
#include "surf/network_gtnets_private.h"

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

void TRACE_surf_host_set_power(double date, const char *resource, double power)
{
  if (!TRACE_is_enabled())
    return;

  container_t container = PJ_container_get(resource);
  type_t type = PJ_type_get ("power", container->type);
  new_pajeSetVariable(date, container, type, power);
}

void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth)
{
  if (!TRACE_is_enabled())
    return;

  container_t container = PJ_container_get(resource);
  type_t type = PJ_type_get ("bandwidth", container->type);
  new_pajeSetVariable(date, container, type, bandwidth);
}

//FIXME: this function is not used (latency availability traces support exists in surf network models?)
void TRACE_surf_link_set_latency(double date, const char *resource, double latency)
{
  if (!TRACE_is_enabled())
    return;

  container_t container = PJ_container_get(resource);
  type_t type = PJ_type_get ("latency", container->type);
  new_pajeSetVariable(date, container, type, latency);
}

/* to trace gtnets */
void TRACE_surf_gtnets_communicate(void *action, const char *src, const char *dst)
{
  surf_action_network_GTNETS_t gtnets_action = (surf_action_network_GTNETS_t)action;
  gtnets_action->src_name = xbt_strdup (src);
  gtnets_action->dst_name = xbt_strdup (dst);
}

void TRACE_surf_gtnets_destroy(void *action)
{
  surf_action_network_GTNETS_t gtnets_action = (surf_action_network_GTNETS_t)action;
  xbt_free (gtnets_action->src_name);
  xbt_free (gtnets_action->dst_name);
}

void TRACE_surf_action(surf_action_t surf_action, const char *category)
{
  if (!TRACE_is_enabled())
    return;
  if (!TRACE_categorized ())
    return;
  if (!category)
    return;

  surf_action->category = xbt_strdup(category);
}
#endif /* HAVE_TRACING */
