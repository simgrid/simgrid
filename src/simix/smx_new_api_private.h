/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ****************************************************************************************** */
/* TUTORIAL: New API                                                                        */
/* ****************************************************************************************** */
#ifndef _SIMIX_NEW_API_PRIVATE_H
#define _SIMIX_NEW_API_PRIVATE_H

#include "simgrid/simix.h"
#include "smx_smurf_private.h"

void SIMIX_pre_new_api_fct(smx_simcall_t simcall);
smx_action_t SIMIX_new_api_fct(smx_process_t process, const char* param1, double param2);

void SIMIX_post_new_api(smx_action_t action);
void SIMIX_new_api_destroy(smx_action_t action);
void SIMIX_new_api_finish(smx_action_t action);

#endif
