/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "surf/storage_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix,
                                "Logging specific to SIMIX (io)");


//SIMIX FILE READ
void SIMIX_pre_file_read(smx_simcall_t simcall, void *ptr, size_t size,
		         size_t nmemb, smx_file_t fd)
{
  smx_action_t action = SIMIX_file_read(simcall->issuer, ptr, size, nmemb, fd);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_read(smx_process_t process, void* ptr, size_t size,
                             size_t nmemb, smx_file_t fd)
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
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  action->io.surf_io =
      surf_workstation_model->extension.workstation.read(host, ptr, size, nmemb,
                                                         fd->surf_file);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}

//SIMIX FILE WRITE
void SIMIX_pre_file_write(smx_simcall_t simcall, const void *ptr, size_t size,
	                  size_t nmemb, smx_file_t fd)
{
  smx_action_t action = SIMIX_file_write(simcall->issuer, ptr, size, nmemb, fd);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_write(smx_process_t process, const void* ptr,
                              size_t size, size_t nmemb, smx_file_t fd)
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
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  action->io.surf_io =
      surf_workstation_model->extension.workstation.write(host, ptr, size,
                                                          nmemb, fd->surf_file);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}

//SIMIX FILE OPEN
void SIMIX_pre_file_open(smx_simcall_t simcall, const char* mount,
                         const char* path)
{
  smx_action_t action = SIMIX_file_open(simcall->issuer, mount, path);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_open(smx_process_t process ,const char* mount,
                             const char* path)
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
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  action->io.surf_io =
      surf_workstation_model->extension.workstation.open(host, mount, path);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}

//SIMIX FILE CLOSE
void SIMIX_pre_file_close(smx_simcall_t simcall, smx_file_t fd)
{
  smx_action_t action = SIMIX_file_close(simcall->issuer, fd);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_close(smx_process_t process, smx_file_t fd)
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
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  action->io.surf_io = surf_workstation_model->extension.workstation.close(host, fd->surf_file);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}


//SIMIX FILE UNLINK
int SIMIX_pre_file_unlink(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_unlink(simcall->issuer, fd);
}

int SIMIX_file_unlink(smx_process_t process, smx_file_t fd)
{
  smx_host_t host = process->smx_host;
  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           sg_host_name(host));
  }

  if (surf_workstation_model->extension.workstation.unlink(host, fd->surf_file)){
    fd->surf_file = NULL;
    return 1;
  } else
    return 0;
}

//SIMIX FILE LS
void SIMIX_pre_file_ls(smx_simcall_t simcall,
                       const char* mount, const char* path)
{
  smx_action_t action = SIMIX_file_ls(simcall->issuer, mount, path);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}
smx_action_t SIMIX_file_ls(smx_process_t process, const char* mount, const char *path)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;
  /* check if the host is active */
  if (surf_workstation_model->extension.workstation.get_state(host) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           sg_host_name(host));
  }

  action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_IO;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->io.host = host;
  action->io.surf_io = surf_workstation_model->extension.workstation.ls(host,mount,path);

  surf_workstation_model->action_data_set(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);
  return action;
}

size_t SIMIX_pre_file_get_size(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_get_size(simcall->issuer, fd);
}

size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_model->extension.workstation.get_size(host,
      fd->surf_file);
}


void SIMIX_post_io(smx_action_t action)
{
  xbt_fifo_item_t i;
  smx_simcall_t simcall;
//  char* key;
//  xbt_dict_cursor_t cursor = NULL;
//  s_file_stat_t *dst = NULL;
//  s_file_stat_t *src = NULL;

  xbt_fifo_foreach(action->simcalls,i,simcall,smx_simcall_t) {
    switch (simcall->call) {
    case SIMCALL_FILE_OPEN:;
      smx_file_t tmp = xbt_new(s_smx_file_t,1);
      tmp->surf_file = (action->io.surf_io)->file;
      simcall_file_open__set__result(simcall, tmp);
      break;

    case SIMCALL_FILE_CLOSE:
      xbt_free(simcall_file_close__get__fd(simcall));
      simcall_file_close__set__result(simcall, 0);
      break;

    case SIMCALL_FILE_WRITE:
      simcall_file_write__set__result(simcall, (action->io.surf_io)->cost);
      break;

    case SIMCALL_FILE_READ:
      simcall_file_read__set__result(simcall, (action->io.surf_io)->cost);
      break;

    case SIMCALL_FILE_LS:
//      xbt_dict_foreach((action->io.surf_io)->ls_dict,cursor,key, src){
//        // if there is a stat we have to duplicate it
//        if(src){
//          dst = xbt_new0(s_file_stat_t,1);
//          file_stat_copy(src, dst);
//          xbt_dict_set((action->io.surf_io)->ls_dict,key,dst,xbt_free);
//        }
//      }
      simcall_file_ls__set__result(simcall, (action->io.surf_io)->ls_dict);
      break;
    default:
      break;
    }
  }

  switch (surf_workstation_model->action_state_get(action->io.surf_io)) {

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
        xbt_die("Internal error in SIMIX_io_finish: unexpected action state %d",
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
  SIMIX_io_destroy(action);
}
