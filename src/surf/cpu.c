/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "host_private.h"

surf_host_resource_t surf_host_resource = NULL;

static void parse_file(const char *file)
{
}

static void *name_service(const char *name)
{
  return NULL;
}

static surf_action_t action_new(void *callback)
{
  return NULL;
}

static e_surf_action_state_t action_get_state(surf_action_t action)
{
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

static void action_free(surf_action_t * action)
{
  return;
}

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action, e_surf_action_state_t state)
{
  return;
}

static xbt_heap_float_t share_resources(void)
{
  return -1.0;
}

static void solve(xbt_heap_float_t date)
{
  return;
}

static void execute(void *host, xbt_maxmin_float_t size, surf_action_t action)
{
}

static e_surf_host_state_t get_state(void *host)
{
  return SURF_HOST_OFF;
}


surf_host_resource_t surf_host_resource_init(void)
{
  surf_host_resource = xbt_new0(s_surf_host_resource_t,1);

  surf_host_resource->resource.parse_file = parse_file;
  surf_host_resource->resource.name_service = name_service;
  surf_host_resource->resource.action_new=action_new;
  surf_host_resource->resource.action_get_state=surf_action_get_state;
  surf_host_resource->resource.action_free = action_free;
  surf_host_resource->resource.action_cancel = action_cancel;
  surf_host_resource->resource.action_recycle = action_recycle;
  surf_host_resource->resource.action_change_state = action_change_state;
  surf_host_resource->resource.share_resources = share_resources;
  surf_host_resource->resource.solve = solve;

  surf_host_resource->execute = execute;
  surf_host_resource->get_state = get_state;

  return surf_host_resource;
}
