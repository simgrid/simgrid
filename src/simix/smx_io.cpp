/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/dict.h>

#include <simgrid/s4u/host.hpp>

#include <mc/mc.h>

#include "src/surf/surf_interface.hpp"
#include "smx_private.h"

#include "src/kernel/activity/SynchroIo.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix, "Logging specific to SIMIX (io)");


/**
 * \brief Internal function to create a SIMIX storage.
 * \param name name of the storage to create
 * \param storage the SURF storage to encapsulate
 * \param data some user data (may be nullptr)
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
  smx_storage_priv_t storage = static_cast<smx_storage_priv_t>(s);

  xbt_assert((storage != nullptr), "Invalid parameters");
  if (storage->data)
    free(storage->data);

  /* Clean storage structure */
  free(storage);
}

//SIMIX FILE READ
void simcall_HANDLER_file_read(smx_simcall_t simcall, smx_file_t fd, sg_size_t size, sg_host_t host)
{
  smx_synchro_t synchro = SIMIX_file_read(fd, size, host);
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;
}

smx_synchro_t SIMIX_file_read(smx_file_t fd, sg_size_t size, sg_host_t host)
{
  /* check if the host is active */
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot call this function", sg_host_get_name(host));


  simgrid::simix::Io *synchro = new simgrid::simix::Io();
  synchro->host = host;
  synchro->surf_io = surf_host_read(host, fd->surf_file, size);

  synchro->surf_io->setData(synchro);
  XBT_DEBUG("Create io synchro %p", synchro);

  return synchro;
}

//SIMIX FILE WRITE
void simcall_HANDLER_file_write(smx_simcall_t simcall, smx_file_t fd, sg_size_t size, sg_host_t host)
{
  smx_synchro_t synchro = SIMIX_file_write(fd,  size, host);
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;
}

smx_synchro_t SIMIX_file_write(smx_file_t fd, sg_size_t size, sg_host_t host)
{
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot call this function", sg_host_get_name(host));

  simgrid::simix::Io *synchro = new simgrid::simix::Io();
  synchro->host = host;
  synchro->surf_io = surf_host_write(host, fd->surf_file, size);
  synchro->surf_io->setData(synchro);
  XBT_DEBUG("Create io synchro %p", synchro);

  return synchro;
}

//SIMIX FILE OPEN
void simcall_HANDLER_file_open(smx_simcall_t simcall, const char* fullpath, sg_host_t host)
{
  smx_synchro_t synchro = SIMIX_file_open(fullpath, host);
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;
}

smx_synchro_t SIMIX_file_open(const char* fullpath, sg_host_t host)
{
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot call this function", sg_host_get_name(host));

  simgrid::simix::Io *synchro = new simgrid::simix::Io();
  synchro->host = host;
  synchro->surf_io = surf_host_open(host, fullpath);
  synchro->surf_io->setData(synchro);
  XBT_DEBUG("Create io synchro %p", synchro);

  return synchro;
}

//SIMIX FILE CLOSE
void simcall_HANDLER_file_close(smx_simcall_t simcall, smx_file_t fd, sg_host_t host)
{
  smx_synchro_t synchro = SIMIX_file_close(fd, host);
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;
}

smx_synchro_t SIMIX_file_close(smx_file_t fd, sg_host_t host)
{
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot call this function", sg_host_get_name(host));

  simgrid::simix::Io *synchro = new simgrid::simix::Io();
  synchro->host = host;
  synchro->surf_io = surf_host_close(host, fd->surf_file);
  synchro->surf_io->setData(synchro);
  XBT_DEBUG("Create io synchro %p", synchro);

  return synchro;
}

//SIMIX FILE UNLINK
int SIMIX_file_unlink(smx_file_t fd, sg_host_t host)
{
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot call this function", sg_host_get_name(host));

  int res = surf_host_unlink(host, fd->surf_file);
  xbt_free(fd);
  return res;
}

sg_size_t simcall_HANDLER_file_get_size(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_get_size(simcall->issuer, fd);
}

sg_size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd)
{
  sg_host_t host = process->host;
  return  surf_host_get_size(host, fd->surf_file);
}

sg_size_t simcall_HANDLER_file_tell(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_tell(simcall->issuer, fd);
}

sg_size_t SIMIX_file_tell(smx_process_t process, smx_file_t fd)
{
  sg_host_t host = process->host;
  return  surf_host_file_tell(host, fd->surf_file);
}


xbt_dynar_t simcall_HANDLER_file_get_info(smx_simcall_t simcall, smx_file_t fd)
{
  return SIMIX_file_get_info(simcall->issuer, fd);
}

xbt_dynar_t SIMIX_file_get_info(smx_process_t process, smx_file_t fd)
{
  sg_host_t host = process->host;
  return  surf_host_get_info(host, fd->surf_file);
}

int simcall_HANDLER_file_seek(smx_simcall_t simcall, smx_file_t fd, sg_offset_t offset, int origin)
{
  return SIMIX_file_seek(simcall->issuer, fd, offset, origin);
}

int SIMIX_file_seek(smx_process_t process, smx_file_t fd, sg_offset_t offset, int origin)
{
  sg_host_t host = process->host;
  return  surf_host_file_seek(host, fd->surf_file, offset, origin);
}

int simcall_HANDLER_file_move(smx_simcall_t simcall, smx_file_t file, const char* fullpath)
{
  return SIMIX_file_move(simcall->issuer, file, fullpath);
}

int SIMIX_file_move(smx_process_t process, smx_file_t file, const char* fullpath)
{
  sg_host_t host = process->host;
  return  surf_host_file_move(host, file->surf_file, fullpath);
}

sg_size_t SIMIX_storage_get_size(smx_storage_t storage){
  return surf_storage_get_size(storage);
}

sg_size_t simcall_HANDLER_storage_get_free_size(smx_simcall_t simcall, smx_storage_t storage)
{
  return SIMIX_storage_get_free_size(simcall->issuer, storage);
}

sg_size_t SIMIX_storage_get_free_size(smx_process_t process, smx_storage_t storage)
{
  return  surf_storage_get_free_size(storage);
}

sg_size_t simcall_HANDLER_storage_get_used_size(smx_simcall_t simcall, smx_storage_t storage)
{
  return SIMIX_storage_get_used_size(simcall->issuer, storage);
}

sg_size_t SIMIX_storage_get_used_size(smx_process_t process, smx_storage_t storage)
{
  return  surf_storage_get_used_size(storage);
}

xbt_dict_t SIMIX_storage_get_properties(smx_storage_t storage){
  return surf_storage_get_properties(storage);
}

const char* SIMIX_storage_get_name(smx_storage_t storage){
  return sg_storage_name(storage);
}

xbt_dict_t SIMIX_storage_get_content(smx_storage_t storage){
  return surf_storage_get_content(storage);
}

const char* SIMIX_storage_get_host(smx_storage_t storage){
  return surf_storage_get_host(storage);
}

void SIMIX_io_destroy(smx_synchro_t synchro)
{
  simgrid::simix::Io *io = static_cast<simgrid::simix::Io*>(synchro);
  XBT_DEBUG("Destroy synchro %p", synchro);
  if (io->surf_io)
    io->surf_io->unref();
  delete io;
}

void SIMIX_io_finish(smx_synchro_t synchro)
{
  for (smx_simcall_t simcall : synchro->simcalls) {
    switch (synchro->state) {
      case SIMIX_DONE:
        /* do nothing, synchro done */
        break;
      case SIMIX_FAILED:
        SMX_EXCEPTION(simcall->issuer, io_error, 0, "IO failed");
        break;
      case SIMIX_CANCELED:
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;
      default:
        xbt_die("Internal error in SIMIX_io_finish: unexpected synchro state %d", static_cast<int>(synchro->state));
    }

    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro = nullptr;
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_io_destroy(synchro);
}
