/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_BASE_H
#define MC_BASE_H

#include <simgrid/simix.h>
#include "simgrid_config.h"
#include "internal_config.h"
#include "../simix/smx_private.h"

SG_BEGIN_DECL()

int MC_request_is_enabled(smx_simcall_t req);
int MC_request_is_visible(smx_simcall_t req);
void MC_wait_for_requests(void);

extern double *mc_time;

SG_END_DECL()

#endif
