/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"

static xbt_heap_float_t NOW=0;

xbt_dynar_t resource_list = NULL;
tmgr_history_t history = NULL;
lmm_system_t maxmin_system = NULL;

e_surf_action_state_t surf_action_get_state(surf_action_t action)
{
  surf_action_state_t action_state = &(action->resource_type->states); 
  
  if(action->state_set == action_state->ready_action_set)
    return SURF_ACTION_READY;
  if(action->state_set == action_state->running_action_set)
    return SURF_ACTION_RUNNING;
  if(action->state_set == action_state->failed_action_set)
    return SURF_ACTION_FAILED;
  if(action->state_set == action_state->done_action_set)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

void surf_action_free(surf_action_t * action)
{
  (*action)->resource_type->action_cancel(*action);
  xbt_free(*action);
  *action=NULL;
}

void surf_action_change_state(surf_action_t action, e_surf_action_state_t state)
{
  surf_action_state_t action_state = &(action->resource_type->states); 

  xbt_swag_remove(action, action->state_set);

  if(state == SURF_ACTION_READY) 
    action->state_set = action_state->ready_action_set;
  else if(state == SURF_ACTION_RUNNING)
    action->state_set = action_state->running_action_set;
  else if(state == SURF_ACTION_FAILED)
    action->state_set = action_state->failed_action_set;
  else if(state == SURF_ACTION_DONE)
    action->state_set = action_state->done_action_set;
  else action->state_set = NULL;

  if(action->state_set) xbt_swag_insert(action, action->state_set);
}

void surf_init(void)
{
  if(!resource_list) resource_list = xbt_dynar_new(sizeof(surf_resource_t), NULL);
  if(!history) history = tmgr_history_new();
  if(!maxmin_system) maxmin_system = lmm_system_new();
}

xbt_heap_float_t surf_solve(void)
{
  xbt_heap_float_t min = -1.0;
  xbt_heap_float_t next_event_date = -1.0;
  xbt_heap_float_t resource_next_action_end = -1.0;
  xbt_maxmin_float_t value = -1.0;
  surf_resource_t resource = NULL;
  int i;

  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if(next_event_date > NOW) break;
    while (tmgr_history_get_next_event_leq(history, next_event_date,
					   &value, (void **) &resource)) {
      if(surf_cpu_resource->resource.resource_used(resource)) {
	min = next_event_date-NOW;
      }
    }
  }

  xbt_dynar_foreach (resource_list,i,resource) {
    resource_next_action_end = resource->share_resources(NOW);
    if((min<0) || (resource_next_action_end<min)) 
      min = resource_next_action_end;
  }

  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if(next_event_date > NOW+min) break;
    while (tmgr_history_get_next_event_leq(history, next_event_date,
					   &value, (void **) &resource)) {
      if(surf_cpu_resource->resource.resource_used(resource)) {
	min = next_event_date-NOW;
      }
    }
  }

  xbt_dynar_foreach (resource_list,i,resource) {
    resource->update_state(NOW, min);
  }

  NOW=NOW+min;

  return min;
}

xbt_heap_float_t surf_get_clock(void)
{
  return NOW;
}
