/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_IO_PRIVATE_H
#define _SIMIX_IO_PRIVATE_H

#include <xbt/base.h>

#include "simgrid/simix.h"
#include "popping_private.h"

/** @brief Storage datatype */
typedef struct s_smx_storage_priv {
  void *data;              /**< @brief user data */
} s_smx_storage_priv_t;


static inline smx_storage_priv_t SIMIX_storage_priv(smx_storage_t storage){
  return (smx_storage_priv_t) xbt_lib_get_level(storage, SIMIX_STORAGE_LEVEL);
}

XBT_PRIVATE smx_storage_t SIMIX_storage_create(const char *name, void *storage, void *data);
XBT_PRIVATE void SIMIX_storage_destroy(void *s);
XBT_PRIVATE smx_synchro_t SIMIX_file_read(smx_file_t fd, sg_size_t size, sg_host_t host);
XBT_PRIVATE smx_synchro_t SIMIX_file_write(smx_file_t fd, sg_size_t size, sg_host_t host);
XBT_PRIVATE smx_synchro_t SIMIX_file_open(const char* fullpath, sg_host_t host);
XBT_PRIVATE smx_synchro_t SIMIX_file_close(smx_file_t fd, sg_host_t host);
XBT_PRIVATE int SIMIX_file_unlink(smx_file_t fd, sg_host_t host);
XBT_PRIVATE sg_size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd);
XBT_PRIVATE sg_size_t SIMIX_file_tell(smx_process_t process, smx_file_t fd);
XBT_PRIVATE xbt_dynar_t SIMIX_file_get_info(smx_process_t process, smx_file_t fd);
XBT_PRIVATE int SIMIX_file_seek(smx_process_t process, smx_file_t fd, sg_offset_t offset, int origin);
XBT_PRIVATE int SIMIX_file_move(smx_process_t process, smx_file_t fd, const char* fullpath);

XBT_PRIVATE sg_size_t SIMIX_storage_get_free_size(smx_process_t process, smx_storage_t storage);
XBT_PRIVATE sg_size_t SIMIX_storage_get_used_size(smx_process_t process, smx_storage_t storage);

XBT_PRIVATE xbt_dict_t SIMIX_storage_get_properties(smx_storage_t storage);

XBT_PRIVATE void SIMIX_post_io(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_io_destroy(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_io_finish(smx_synchro_t synchro);

#endif
