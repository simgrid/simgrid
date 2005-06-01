/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_timer_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(timer, surf,
				"Logging specific to the SURF timer module");

surf_timer_resource_t surf_timer_resource = NULL;
static tmgr_trace_t empty_trace = NULL;
static xbt_dict_t command_set = NULL;

static void timer_free(void *timer)
{
  free(timer);
}

static command_t command_new(void *fun, void* args)
{
  command_t command = xbt_new0(s_command_t, 1);

  command->resource = (surf_resource_t) surf_timer_resource;
  command->fun = fun;
  command->args = args;
/*   command->name = name; */
/*   command->power_scale = power_scale; */
/*   xbt_assert0(command->power_scale>0,"Power has to be >0"); */
/*   command->power_current = power_initial; */
/*   if (power_trace) */
/*     command->power_event = */
/* 	tmgr_history_add_trace(history, power_trace, 0.0, 0, command); */

/*   command->state_current = state_initial; */
/*   if (state_trace) */
/*     command->state_event = */
/* 	tmgr_history_add_trace(history, state_trace, 0.0, 0, command); */

/*   command->constraint = */
/*       lmm_constraint_new(maxmin_system, command, */
/* 			 command->power_current * command->power_scale); */

/*   xbt_dict_set(command_set, name, command, command_free); */

  return command;
}

static void parse_timer(void)
{
}

static void parse_file(const char *file)
{
}

static void *name_service(const char *name)
{
  DIE_IMPOSSIBLE;
  return NULL;
}

static const char *get_resource_name(void *resource_id)
{
  DIE_IMPOSSIBLE;
  return "";
}

static int resource_used(void *resource_id)
{
  return 1;
}

static void action_free(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

static void action_cancel(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

static void action_recycle(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  DIE_IMPOSSIBLE;
  return;
}

static double share_resources(double now)
{
  return -1.0;
}

static void update_actions_state(double now, double delta)
{
  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  double value)
{
  command_t command = id;

  // Move this command to the list of commands to execute

  return;
}

static void set(double date, void *function, void *arg)
{
  command_t command = NULL;

  command = command_new(function, arg);

  tmgr_history_add_trace(history, empty_trace, date, 0, command);
  
}


static int get(void **function, void **arg)
{
  return 0;
}

static void action_suspend(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return 0;
}

static void finalize(void)
{
  xbt_dict_free(&command_set);

  xbt_swag_free(surf_timer_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_timer_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_timer_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_timer_resource->common_public->states.done_action_set);
  free(surf_timer_resource->common_public);
  free(surf_timer_resource->common_private);
  free(surf_timer_resource->extension_public);

  free(surf_timer_resource);
  surf_timer_resource = NULL;
}

static void surf_timer_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_timer_resource = xbt_new0(s_surf_timer_resource_t, 1);

  surf_timer_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_timer_resource->common_public = xbt_new0(s_surf_resource_public_t, 1);

  surf_timer_resource->extension_public =
      xbt_new0(s_surf_timer_resource_extension_public_t, 1);

  surf_timer_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_timer_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_timer_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_timer_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_timer_resource->common_public->name_service = name_service;
  surf_timer_resource->common_public->get_resource_name = get_resource_name;
  surf_timer_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_timer_resource->common_public->action_free = action_free;
  surf_timer_resource->common_public->action_cancel = action_cancel;
  surf_timer_resource->common_public->action_recycle = action_recycle;
  surf_timer_resource->common_public->action_change_state =
      action_change_state;
  surf_timer_resource->common_public->action_set_data = surf_action_set_data;
  surf_timer_resource->common_public->name = "TIMER";

  surf_timer_resource->common_private->resource_used = resource_used;
  surf_timer_resource->common_private->share_resources = share_resources;
  surf_timer_resource->common_private->update_actions_state =
      update_actions_state;
  surf_timer_resource->common_private->update_resource_state =
      update_resource_state;
  surf_timer_resource->common_private->finalize = finalize;

  surf_timer_resource->common_public->suspend = action_suspend;
  surf_timer_resource->common_public->resume = action_resume;
  surf_timer_resource->common_public->is_suspended = action_is_suspended;

  surf_timer_resource->extension_public->set = set;
  surf_timer_resource->extension_public->get = get;

  command_set = xbt_dict_new();
  empty_trace = tmgr_empty_trace_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

void surf_timer_resource_init(const char *filename)
{
  if (surf_timer_resource)
    return;
  surf_timer_resource_init_internal();
  xbt_dynar_push(resource_list, &surf_timer_resource);
}
