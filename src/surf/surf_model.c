
/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "xbt/dict.h"

static void void_die_impossible_paction(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

static int int_die_impossible_paction(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

/** @brief initialize common datastructures to all models */
surf_model_t surf_model_init(void)
{
  s_surf_action_t action;
  surf_model_t model = xbt_new0(s_surf_model_t, 1);

  model->model_private = xbt_new0(s_surf_model_private_t, 1);

  model->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  model->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  model->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  model->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  model->resource_set = xbt_dict_new();

  model->action_unref = int_die_impossible_paction;
  model->action_cancel = void_die_impossible_paction;
  model->action_recycle = void_die_impossible_paction;

  model->action_state_get = surf_action_state_get;
  model->action_state_set = surf_action_state_set;
  model->action_get_start_time = surf_action_get_start_time;
  model->action_get_finish_time = surf_action_get_finish_time;
  model->action_data_set = surf_action_data_set;


  return model;
}

/** @brief finalize common datastructures to all models */
void surf_model_exit(surf_model_t model)
{
  xbt_swag_free(model->states.ready_action_set);
  xbt_swag_free(model->states.running_action_set);
  xbt_swag_free(model->states.failed_action_set);
  xbt_swag_free(model->states.done_action_set);
  xbt_dict_free(&model->resource_set);
  free(model->model_private);
  free(model);
}

void *surf_model_resource_by_name(surf_model_t model, const char *name)
{
  return xbt_dict_get_or_null(model->resource_set, name);
}
