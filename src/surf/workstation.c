/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "workstation_private.h"
/* #include "cpu_private.h" */
/* #include "network_private.h" */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(workstation, surf,
				"Logging specific to the SURF workstation module");

surf_workstation_resource_t surf_workstation_resource = NULL;

static xbt_dict_t workstation_set = NULL;

static workstation_t workstation_new(const char *name)
{
  return NULL;
}

static void *name_service(const char *name)
{
  return NULL;
}

static const char *get_resource_name(void *resource_id)
{
  return NULL;
}

static int resource_used(void *resource_id)
{
  return 0;
}

static void action_free(surf_action_t action)
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

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static xbt_heap_float_t share_resources(xbt_heap_float_t now)
{
  return -1.0;
}


static void update_actions_state(xbt_heap_float_t now,
				 xbt_heap_float_t delta)
{
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  xbt_maxmin_float_t value)
{
  return;
}

static surf_action_t communicate(void *workstation_src, 
				 void *workstation_dst,
				 xbt_maxmin_float_t size)
{
  return NULL;
}

static surf_action_t execute(void *workstation, xbt_maxmin_float_t size)
{
  return NULL;
}

static e_surf_cpu_state_t get_state(void *workstation)
{
  return SURF_CPU_OFF;
}

static void finalize(void)
{
}

static void surf_workstation_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_resource = xbt_new0(s_surf_workstation_resource_t, 1);

  surf_workstation_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_workstation_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
/*   surf_workstation_resource->extension_private = xbt_new0(s_surf_workstation_resource_extension_private_t,1); */
  surf_workstation_resource->extension_public =
      xbt_new0(s_surf_workstation_resource_extension_public_t, 1);

  surf_workstation_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_resource->common_public->name_service = name_service;
  surf_workstation_resource->common_public->get_resource_name =
      get_resource_name;
  surf_workstation_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_resource->common_public->action_free = action_free;
  surf_workstation_resource->common_public->action_cancel = action_cancel;
  surf_workstation_resource->common_public->action_recycle = action_recycle;
  surf_workstation_resource->common_public->action_change_state =
      action_change_state;

  surf_workstation_resource->common_private->resource_used = resource_used;
  surf_workstation_resource->common_private->share_resources = share_resources;
  surf_workstation_resource->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_resource->common_private->update_resource_state =
      update_resource_state;
  surf_workstation_resource->common_private->finalize = finalize;

  surf_workstation_resource->extension_public->communicate = communicate;
  surf_workstation_resource->extension_public->execute = execute;

  workstation_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

void surf_workstation_resource_init(const char *filename)
{
  surf_workstation_resource_init_internal();
/*   parse_file(filename); */
  xbt_dynar_push(resource_list, &surf_workstation_resource);
}
