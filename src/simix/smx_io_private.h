/* Copyright (c) 2007-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_IO_PRIVATE_H
#define _SIMIX_IO_PRIVATE_H

#include "simgrid/simix.h"
#include "popping_private.h"

/** @brief Storage datatype */
typedef struct s_smx_storage_priv {
  void *data;              /**< @brief user data */
} s_smx_storage_priv_t;


static inline smx_storage_priv_t SIMIX_storage_priv(smx_storage_t storage){
  return (smx_storage_priv_t) xbt_lib_get_level(storage, SIMIX_STORAGE_LEVEL);
}

smx_storage_t SIMIX_storage_create(const char *name, void *storage, void *data);
void SIMIX_storage_destroy(void *s);
smx_synchro_t SIMIX_file_read(smx_file_t fd, sg_size_t size, smx_host_t host);
smx_synchro_t SIMIX_file_write(smx_file_t fd, sg_size_t size, smx_host_t host);
smx_synchro_t SIMIX_file_open(const char* fullpath, smx_host_t host);
smx_synchro_t SIMIX_file_close(smx_file_t fd, smx_host_t host);
int SIMIX_file_unlink(smx_file_t fd, smx_host_t host);
sg_size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd);
sg_size_t SIMIX_file_tell(smx_process_t process, smx_file_t fd);
xbt_dynar_t SIMIX_file_get_info(smx_process_t process, smx_file_t fd);
int SIMIX_file_seek(smx_process_t process, smx_file_t fd, sg_offset_t offset, int origin);
int SIMIX_file_move(smx_process_t process, smx_file_t fd, const char* fullpath);

sg_size_t SIMIX_storage_get_free_size(smx_process_t process, smx_storage_t storage);
sg_size_t SIMIX_storage_get_used_size(smx_process_t process, smx_storage_t storage);

xbt_dict_t SIMIX_storage_get_properties(smx_storage_t storage);

xbt_dict_t SIMIX_storage_get_content(smx_storage_t storage);

void SIMIX_post_io(smx_synchro_t synchro);
void SIMIX_io_destroy(smx_synchro_t synchro);
void SIMIX_io_finish(smx_synchro_t synchro);

#endif
