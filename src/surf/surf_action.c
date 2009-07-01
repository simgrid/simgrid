/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

const char *surf_action_state_names[6] = {
  "SURF_ACTION_READY",
  "SURF_ACTION_RUNNING",
  "SURF_ACTION_FAILED",
  "SURF_ACTION_DONE",
  "SURF_ACTION_TO_FREE",
  "SURF_ACTION_NOT_IN_THE_SYSTEM"
};

e_surf_action_state_t surf_action_state_get(surf_action_t action)
{
  surf_action_state_t action_state = &(action->model_type->states);

  if (action->state_set == action_state->ready_action_set)
    return SURF_ACTION_READY;
  if (action->state_set == action_state->running_action_set)
    return SURF_ACTION_RUNNING;
  if (action->state_set == action_state->failed_action_set)
    return SURF_ACTION_FAILED;
  if (action->state_set == action_state->done_action_set)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

double surf_action_get_start_time(surf_action_t action)
{
  return action->start;
}

double surf_action_get_finish_time(surf_action_t action)
{
  return action->finish;
}

void surf_action_free(surf_action_t * action)
{
  (*action)->model_type->action_cancel(*action);
  free(*action);
  *action = NULL;
}

void surf_action_state_set(surf_action_t action,
                              e_surf_action_state_t state)
{
  surf_action_state_t action_state = &(action->model_type->states);
  XBT_IN2("(%p,%s)", action, surf_action_state_names[state]);
  xbt_swag_remove(action, action->state_set);

  if (state == SURF_ACTION_READY)
    action->state_set = action_state->ready_action_set;
  else if (state == SURF_ACTION_RUNNING)
    action->state_set = action_state->running_action_set;
  else if (state == SURF_ACTION_FAILED)
    action->state_set = action_state->failed_action_set;
  else if (state == SURF_ACTION_DONE)
    action->state_set = action_state->done_action_set;
  else
    action->state_set = NULL;

  if (action->state_set)
    xbt_swag_insert(action, action->state_set);
  XBT_OUT;
}

void surf_action_data_set(surf_action_t action, void *data)
{
  action->data = data;
}
