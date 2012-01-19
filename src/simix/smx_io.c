/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix,
                                "Logging specific to SIMIX (io)");

void SIMIX_pre_file_read(smx_req_t req)
{
  smx_action_t action = SIMIX_file_read(req->issuer, req->file_read.name);
  xbt_fifo_push(action->request_list, req);
  req->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_read(smx_process_t process, char* name)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host->host) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  //  TODO in surf model disk???
  //  action->io.surf_io = surf_workstation_model->extension.disk.read(host->host, name),
    action->io.surf_io = surf_workstation_model->extension.workstation.sleep(host->host, 1.0);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}

void SIMIX_post_file_read(smx_action_t action)
{
  smx_req_t req;

  while ((req = xbt_fifo_shift(action->request_list))) {

    switch(surf_workstation_model->action_state_get(action->io.surf_io)){
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
  }
  /* If there are requests associated with the action, then answer them */
  if (xbt_fifo_size(action->request_list))
	  SIMIX_io_finish(action);
}

void SIMIX_io_destroy(smx_action_t action)
{
  XBT_DEBUG("Destroy action %p", action);
  if (action->io.surf_io)
    action->io.surf_io->model_type->action_unref(action->io.surf_io);
  xbt_mallocator_release(simix_global->action_mallocator, action);
}

void SIMIX_io_finish(smx_action_t action)
{
  volatile xbt_fifo_item_t item;
  smx_req_t req;

  xbt_fifo_foreach(action->request_list, item, req, smx_req_t) {

    switch (action->state) {

      case SIMIX_DONE:
        /* do nothing, action done */
        break;

      case SIMIX_FAILED:
        TRY {
          THROWF(io_error, 0, "IO failed");
        }
	CATCH(req->issuer->running_ctx->exception) {
	  req->issuer->doexception = 1;
	}
      break;

      case SIMIX_CANCELED:
        TRY {
          THROWF(cancel_error, 0, "Canceled");
        }
	CATCH(req->issuer->running_ctx->exception) {
	  req->issuer->doexception = 1;
        }
	break;

      default:
        xbt_die("Internal error in SIMIX_io_finish: unexpected action state %d",
            action->state);
    }
    req->issuer->waiting_action = NULL;
    SIMIX_request_answer(req);
  }

  /* We no longer need it */
  SIMIX_io_destroy(action);
}
