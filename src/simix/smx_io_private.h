/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_IO_PRIVATE_H
#define _SIMIX_IO_PRIVATE_H

#include "simgrid/simix.h"
#include "smx_smurf_private.h"

void SIMIX_pre_file_read(smx_simcall_t simcall, void *ptr, size_t size,
		         size_t nmemb, smx_file_t stream);
void SIMIX_pre_file_write(smx_simcall_t simcall, const void *ptr, size_t size,
		          size_t nmemb, smx_file_t strea);
void SIMIX_pre_file_open(smx_simcall_t simcall, const char* mount,
		         const char* path, const char* mode);
void SIMIX_pre_file_close(smx_simcall_t simcall, smx_file_t fd);
void SIMIX_pre_file_unlink(smx_simcall_t simcall, smx_file_t fd);
void SIMIX_pre_file_ls(smx_simcall_t simcall,
                       const char* mount, const char* path);
size_t SIMIX_pre_file_get_size(smx_simcall_t simcall, smx_file_t fd);

smx_action_t SIMIX_file_read(smx_process_t process, void* ptr, size_t size, size_t nmemb, smx_file_t stream);
smx_action_t SIMIX_file_write(smx_process_t process, const void* ptr, size_t size, size_t nmemb, smx_file_t stream);
smx_action_t SIMIX_file_open(smx_process_t process, const char* storage, const char* path, const char* mode);
smx_action_t SIMIX_file_close(smx_process_t process, smx_file_t fd);
smx_action_t SIMIX_file_unlink(smx_process_t process, smx_file_t fd);
smx_action_t SIMIX_file_ls(smx_process_t process, const char *mount, const char *path);
size_t SIMIX_file_get_size(smx_process_t process, smx_file_t fd);

void SIMIX_post_io(smx_action_t action);
void SIMIX_io_destroy(smx_action_t action);
void SIMIX_io_finish(smx_action_t action);

// pre prototypes

#endif
