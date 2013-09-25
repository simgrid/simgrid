/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_IO_PRIVATE_H
#define _SIMIX_IO_PRIVATE_H

#include "simgrid/simix.h"
#include "smx_smurf_private.h"

/** @brief Storage datatype */
typedef struct s_smx_storage_priv {
  void *data;              /**< @brief user data */
} s_smx_storage_priv_t;


static inline smx_storage_priv_t SIMIX_storage_priv(smx_storage_t storage){
  return xbt_lib_get_level(storage, SIMIX_STORAGE_LEVEL);
}

smx_storage_t SIMIX_storage_create(const char *name, void *storage, void *data);
void* SIMIX_pre_file_get_data(smx_simcall_t simcall,smx_file_t fd);
void SIMIX_pre_file_set_data(smx_simcall_t simcall, smx_file_t fd, void *data);
void SIMIX_pre_file_read(smx_simcall_t simcall, size_t size, smx_file_t fd);
void SIMIX_pre_file_write(smx_simcall_t simcall, size_t size, smx_file_t fd);
void SIMIX_pre_file_open(smx_simcall_t simcall, const char* mount,
		         const char* path);
void SIMIX_pre_file_close(smx_simcall_t simcall, smx_file_t fd);
int SIMIX_pre_file_unlink(smx_simcall_t simcall, smx_file_t fd);
void SIMIX_pre_file_ls(smx_simcall_t simcall,
                       const char* mount, const char* path);
size_t SIMIX_pre_file_get_size(smx_simcall_t simcall, smx_file_t fd);
xbt_dynar_t SIMIX_pre_file_get_info(smx_simcall_t simcall, smx_file_t fd);

void* SIMIX_file_get_data(smx_file_t fd);
void SIMIX_file_set_data(smx_file_t fd, void *data);
smx_action_t SIMIX_file_read(smx_process_t process, size_t size,
                             smx_file_t fd);
smx_action_t SIMIX_file_write(smx_process_t process, size_t size, 
			      smx_file_t fd);
smx_action_t SIMIX_file_open(smx_process_t process, const char* storage,
                             const char* path);
smx_action_t SIMIX_file_close(smx_process_t process, smx_file_t fd);
int SIMIX_file_unlink(smx_process_t process, smx_file_t fd);
smx_action_t SIMIX_file_ls(smx_process_t process, const char *mount,
                           const char *path);
size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd);
xbt_dynar_t SIMIX_file_get_info(smx_process_t process, smx_file_t fd);

size_t SIMIX_pre_storage_get_free_size(smx_simcall_t simcall,const char* name);
size_t SIMIX_storage_get_free_size(smx_process_t process,const char* name);

size_t SIMIX_pre_storage_get_used_size(smx_simcall_t simcall,const char* name);
size_t SIMIX_storage_get_used_size(smx_process_t process,const char* name);

xbt_dict_t SIMIX_storage_get_properties(smx_storage_t storage);
xbt_dict_t SIMIX_pre_storage_get_properties(smx_simcall_t, smx_storage_t);

void SIMIX_pre_storage_set_data(smx_simcall_t, smx_storage_t, void*);
void* SIMIX_pre_storage_get_data(smx_simcall_t, smx_storage_t);

xbt_dict_t SIMIX_pre_storage_get_content(smx_simcall_t simcall, smx_storage_t storage);
xbt_dict_t SIMIX_storage_get_content(smx_storage_t storage);

const char* SIMIX_pre_storage_get_name(smx_simcall_t simcall, smx_storage_t storage);

void SIMIX_post_io(smx_action_t action);
void SIMIX_io_destroy(smx_action_t action);
void SIMIX_io_finish(smx_action_t action);

// pre prototypes

#endif
