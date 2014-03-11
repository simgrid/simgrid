/* Copyright (c) 2007-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
//#include "surf/storage_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix,
                                "Logging specific to SIMIX (io)");


/**
 * \brief Internal function to create a SIMIX storage.
 * \param name name of the storage to create
 * \param storage the SURF storage to encapsulate
 * \param data some user data (may be NULL)
 */
smx_storage_t SIMIX_storage_create(const char *name, void *storage, void *data)
{
  smx_storage_priv_t smx_storage = xbt_new0(s_smx_storage_priv_t, 1);

  smx_storage->data = data;

  /* Update global variables */
  xbt_lib_set(storage_lib,name,SIMIX_STORAGE_LEVEL,smx_storage);
  return xbt_lib_get_elm_or_null(storage_lib, name);
}

/**
 * \brief Internal function to destroy a SIMIX storage.
 *
 * \param s the host to destroy (a smx_storage_t)
 */
void SIMIX_storage_destroy(void *s)
{
  smx_storage_priv_t storage = (smx_storage_priv_t) s;

  xbt_assert((storage != NULL), "Invalid parameters");
  if (storage->data)
    free(storage->data);

  /* Clean storage structure */
  free(storage);
}

//SIMIX FILE READ
void SIMIX_pre_file_read(smx_simcall_t simcall, smx_file_t fd, sg_size_t size)
{
  smx_action_t action = SIMIX_file_read(simcall->issuer, fd, size);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_read(smx_process_t process, smx_file_t fd, sg_size_t size)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
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
  action->io.surf_io = surf_workstation_read(host, fd->surf_file, size);

  surf_action_set_data(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);

  return action;
}

//SIMIX FILE WRITE
void SIMIX_pre_file_write(smx_simcall_t simcall, smx_file_t fd, sg_size_t size)
{
  smx_action_t action = SIMIX_file_write(simcall->issuer, fd,  size);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_file_write(smx_process_t process, smx_file_t fd, sg_size_t size)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
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
  action->io.surf_io = surf_workstation_write(host, fd->surf_file, size);

  surf_action_set_data(action->io.surf_io, action);
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
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
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
  action->io.surf_io = surf_workstation_open(host, mount, path);

  surf_action_set_data(action->io.surf_io, action);
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
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
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
  action->io.surf_io = surf_workstation_close(host, fd->surf_file);

  surf_action_set_data(action->io.surf_io, action);
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
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           sg_host_name(host));
  }

  if (surf_workstation_unlink(host, fd->surf_file)){
    xbt_free(fd);
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
  if (surf_resource_get_state(surf_workstation_resource_priv(host)) != SURF_RESOURCE_ON) {
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
  action->io.surf_io = surf_workstation_ls(host,mount,path);

  surf_action_set_data(action->io.surf_io, action);
  XBT_DEBUG("Create io action %p", action);
  return action;
}

sg_size_t SIMIX_pre_file_get_size(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_get_size(simcall->issuer, fd);
}

sg_size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_get_size(host, fd->surf_file);
}

sg_size_t SIMIX_pre_file_tell(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_tell(simcall->issuer, fd);
}

sg_size_t SIMIX_file_tell(smx_process_t process, smx_file_t fd)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_file_tell(host, fd->surf_file);
}


xbt_dynar_t SIMIX_pre_file_get_info(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_get_info(simcall->issuer, fd);
}

xbt_dynar_t SIMIX_file_get_info(smx_process_t process, smx_file_t fd)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_get_info(host, fd->surf_file);
}

int SIMIX_pre_file_seek(smx_simcall_t simcall, smx_file_t fd, sg_size_t offset, int origin)
{
  return SIMIX_file_seek(simcall->issuer, fd, offset, origin);
}

int SIMIX_file_seek(smx_process_t process, smx_file_t fd, sg_size_t offset, int origin)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_file_seek(host, fd->surf_file, offset, origin);
}

void SIMIX_pre_storage_file_rename(smx_simcall_t simcall, smx_storage_t storage, const char* src, const char* dest)
{
  return SIMIX_storage_file_rename(simcall->issuer, storage, src, dest);
}

void SIMIX_storage_file_rename(smx_process_t process, smx_storage_t storage, const char* src, const char* dest)
{
  return  surf_storage_rename(storage, src, dest);
}

sg_size_t SIMIX_pre_storage_get_free_size(smx_simcall_t simcall, const char* name)
{
  return SIMIX_storage_get_free_size(simcall->issuer, name);
}

sg_size_t SIMIX_storage_get_free_size(smx_process_t process, const char* name)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_get_free_size(host, name);
}

sg_size_t SIMIX_pre_storage_get_used_size(smx_simcall_t simcall, const char* name)
{
  return SIMIX_storage_get_used_size(simcall->issuer, name);
}

sg_size_t SIMIX_storage_get_used_size(smx_process_t process, const char* name)
{
  smx_host_t host = process->smx_host;
  return  surf_workstation_get_used_size(host, name);
}

xbt_dict_t SIMIX_pre_storage_get_properties(smx_simcall_t simcall, smx_storage_t storage){
  return SIMIX_storage_get_properties(storage);
}
xbt_dict_t SIMIX_storage_get_properties(smx_storage_t storage){
  xbt_assert((storage != NULL), "Invalid parameters (simix storage is NULL)");
  return surf_resource_get_properties(surf_storage_resource_priv(storage));
}

const char* SIMIX_pre_storage_get_name(smx_simcall_t simcall, smx_storage_t storage){
   return SIMIX_storage_get_name(storage);
}

const char* SIMIX_storage_get_name(smx_storage_t storage){
  xbt_assert((storage != NULL), "Invalid parameters");
  return sg_storage_name(storage);
}

xbt_dict_t SIMIX_pre_storage_get_content(smx_simcall_t simcall, smx_storage_t storage){
  return SIMIX_storage_get_content(storage);
}

xbt_dict_t SIMIX_storage_get_content(smx_storage_t storage){
  xbt_assert((storage != NULL), "Invalid parameters (simix storage is NULL)");
  return surf_storage_get_content(storage);
}

sg_size_t SIMIX_storage_get_size(smx_storage_t storage){
  xbt_assert((storage != NULL), "Invalid parameters (simix storage is NULL)");
  return surf_storage_get_size(storage);
}

const char* SIMIX_pre_storage_get_host(smx_simcall_t simcall, smx_storage_t storage){
   return SIMIX_storage_get_host(storage);
}

const char* SIMIX_storage_get_host(smx_storage_t storage){
  xbt_assert((storage != NULL), "Invalid parameters");
  return surf_storage_get_host(storage);
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
    case SIMCALL_FILE_OPEN: {
      smx_file_t tmp = xbt_new(s_smx_file_t,1);
      tmp->surf_file = surf_storage_action_get_file(action->io.surf_io);
      simcall_file_open__set__result(simcall, tmp);
      break;
    }
    case SIMCALL_FILE_CLOSE:
      xbt_free(simcall_file_close__get__fd(simcall));
      simcall_file_close__set__result(simcall, 0);
      break;
    case SIMCALL_FILE_WRITE:
      simcall_file_write__set__result(simcall, surf_action_get_cost(action->io.surf_io));
      break;

    case SIMCALL_FILE_READ:
      simcall_file_read__set__result(simcall, surf_action_get_cost(action->io.surf_io));
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
      simcall_file_ls__set__result(simcall, surf_storage_action_get_ls_dict(action->io.surf_io));
      break;
    default:
      break;
    }
  }

  switch (surf_action_get_state(action->io.surf_io)) {

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
    surf_action_unref(action->io.surf_io);
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

    if (surf_resource_get_state(surf_workstation_resource_priv(simcall->issuer->smx_host)) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_action = NULL;
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_io_destroy(action);
}
