
/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

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
void surf_model_init(surf_model_t model)
{
  s_surf_action_t action;


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

  model->action_free = int_die_impossible_paction;
  model->action_cancel = void_die_impossible_paction;
  model->action_recycle = void_die_impossible_paction;

}

void *surf_model_resource_by_name(surf_model_t model, const char *name)
{
  return xbt_dict_get_or_null(model->resource_set, name);
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
}
