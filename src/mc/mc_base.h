/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASE_H
#define SIMGRID_MC_BASE_H

#include <xbt/misc.h>
#include <simgrid/simix.h>
#include "simgrid_config.h"
#include "internal_config.h"
#include "../simix/smx_private.h"

// Marker for symbols which should be defined as XBT_PRIVATE but are used in
// unit tests:
#define MC_SHOULD_BE_INTERNAL

SG_BEGIN_DECL()

/** Check if the given simcall can be resolved
 *
 *  \return `TRUE` or `FALSE`
 */
int MC_request_is_enabled(smx_simcall_t req);

/** Check if the given simcall is visible
 *
 *  \return `TRUE` or `FALSE`
 */
int MC_request_is_visible(smx_simcall_t req);

/** Execute everything which is invisible
 *
 *  Execute all the processes that are ready to run and all invisible simcalls
 *  iteratively until there doesn't remain any. At this point, the function
 *  returns to the caller which can handle the visible (and ready) simcalls.
 */
void MC_wait_for_requests(void);

XBT_INTERNAL extern double *mc_time;

SG_END_DECL()

#endif
