/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_IO_PRIVATE_H
#define _SIMIX_IO_PRIVATE_H

#include "simix/datatypes.h"
#include "smurf_private.h"

void SIMIX_pre_file_read(smx_req_t req);
smx_action_t SIMIX_file_read(smx_process_t process, char* name);
void SIMIX_post_io(smx_action_t action);
void SIMIX_io_destroy(smx_action_t action);
void SIMIX_io_finish(smx_action_t action);

#endif
