/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ****************************************************************************************** */
/* TUTORIAL: New API                                                                        */
/* ****************************************************************************************** */
#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_new_api, simix,
                                "Logging specific to SIMIX (new_api)");


//SIMIX NEW MODEL INIT
void SIMIX_pre_new_api_fct(smx_simcall_t simcall)
{
  smx_action_t action = SIMIX_new_api_fct(simcall->issuer,
      simcall->new_api.param1,
      simcall->new_api.param2);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

void SIMIX_post_new_api(smx_action_t action)
{
  xbt_fifo_item_t i;
  smx_simcall_t simcall;

  xbt_fifo_foreach(action->simcalls,i,simcall,smx_simcall_t) {
    switch (simcall->call) {
    case SIMCALL_NEW_API_INIT:
      simcall->new_api.result = 0;
      break;

    default:
      break;
    }
  }

  switch (surf_workstation_model->action_state_get(action->new_api.surf_new_api)) {

    case SURF_ACTION_FAILED:
      action->state = SIMIX_FAILED;
      break;

    case SURF_ACTION_DONE:
      action->state = SIMIX_DONE;
      break;

    default:
      THROW_IMPOSSIBLE;
      break;
  }

  SIMIX_new_api_finish(action);
}

smx_action_t SIMIX_new_api_fct(smx_process_t process, const char* param1, double param2)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           sg_host_name(host));
  }

  action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_NEW_API;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  // Called the function from the new model
  action->new_api.surf_new_api = surf_workstation_model->extension.new_model.fct();

  surf_workstation_model->action_data_set(action->new_api.surf_new_api, action);
  XBT_DEBUG("Create NEW MODEL action %p", action);

  return action;
}

void SIMIX_new_api_destroy(smx_action_t action)
{
  XBT_DEBUG("Destroy action %p", action);
  if (action->new_api.surf_new_api)
    action->new_api.surf_new_api->model_obj->action_unref(action->new_api.surf_new_api);
  xbt_mallocator_release(simix_global->action_mallocator, action);
}

void SIMIX_new_api_finish(smx_action_t action)
{
  xbt_fifo_item_t item;
  smx_simcall_t simcall;

  xbt_fifo_foreach(action->simcalls, item, simcall, smx_simcall_t) {

    switch (action->state) {

      case SIMIX_DONE:
        /* do nothing, action done */
        break;

      case SIMIX_FAILED:
        SMX_EXCEPTION(simcall->issuer, io_error, 0, "IO failed");
        break;

      case SIMIX_CANCELED:
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;

      default:
        xbt_die("Internal error in SIMIX_NEW_MODEL_finish: unexpected action state %d",
            (int)action->state);
    }

    if (surf_workstation_model->extension.
        workstation.get_state(simcall->issuer->smx_host) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_action = NULL;
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_new_api_destroy(action);
}
