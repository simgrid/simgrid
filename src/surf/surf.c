/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "cpu_private.h"

s_surf_global_t surf_global;

e_surf_action_state_t surf_action_get_state(surf_action_t action)
{
  if(action->state_set == surf_global.ready_action_set)
    return SURF_ACTION_READY;
  if(action->state_set == surf_global.running_action_set)
    return SURF_ACTION_RUNNING;
  if(action->state_set == surf_global.failed_action_set)
    return SURF_ACTION_FAILED;
  if(action->state_set == surf_global.done_action_set)
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
  xbt_swag_extract(action, action->state_set);
  if(state == SURF_ACTION_READY) 
    action->state_set = surf_global.ready_action_set;
  else if(state == SURF_ACTION_RUNNING)
    action->state_set = surf_global.running_action_set;
  else if(state == SURF_ACTION_FAILED)
    action->state_set = surf_global.failed_action_set;
  else if(state == SURF_ACTION_DONE)
    action->state_set = surf_global.done_action_set;
  else action->state_set = NULL;

  if(action->state_set) xbt_swag_insert(action, action->state_set);
}

void surf_init(void)
{
  surf_cpu_resource = surf_cpu_resource_init();
}

/* xbt_heap_float_t surf_solve(void) */
/* { */
/* } */

