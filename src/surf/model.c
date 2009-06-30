
/* Copyright (c) 2009 The SimGrid Team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"

static void void_die_impossible_paction(surf_action_t action) {
	DIE_IMPOSSIBLE;
}
static int int_die_impossible_paction(surf_action_t action) {
	DIE_IMPOSSIBLE;
}

/** @brief initialize common datastructures to all models */
void surf_model_init(surf_model_t model) {
	  s_surf_action_t action;


	  model->common_private = xbt_new0(s_surf_model_private_t, 1);

	  model->common_public.states.ready_action_set =
	    xbt_swag_new(xbt_swag_offset(action, state_hookup));
	  model->common_public.states.running_action_set =
	    xbt_swag_new(xbt_swag_offset(action, state_hookup));
	  model->common_public.states.failed_action_set =
	    xbt_swag_new(xbt_swag_offset(action, state_hookup));
	  model->common_public.states.done_action_set =
	    xbt_swag_new(xbt_swag_offset(action, state_hookup));

	  model->common_public.action_free = int_die_impossible_paction;
	  model->common_public.action_cancel = void_die_impossible_paction;
	  model->common_public.action_recycle = void_die_impossible_paction;

}


/** @brief finalize common datastructures to all models */
void surf_model_exit(surf_model_t model) {
	  xbt_swag_free(model->common_public.states.ready_action_set);
	  xbt_swag_free(model->common_public.states.running_action_set);
	  xbt_swag_free(model->common_public.states.failed_action_set);
	  xbt_swag_free(model->common_public.states.done_action_set);
	  free(model->common_private);
}
