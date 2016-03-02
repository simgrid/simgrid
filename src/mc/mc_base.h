/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASE_H
#define SIMGRID_MC_BASE_H

#include <xbt/base.h>
#include "src/simix/popping_private.h" // smx_simcall_t

SG_BEGIN_DECL()

/** Check if the given simcall can be resolved
 *
 *  \return `TRUE` or `FALSE`
 */
XBT_PRIVATE int MC_request_is_enabled(smx_simcall_t req);

/** Check if the given simcall is visible
 *
 *  \return `TRUE` or `FALSE`
 */
XBT_PRIVATE int MC_request_is_visible(smx_simcall_t req);

/** Execute everything which is invisible
 *
 *  Execute all the processes that are ready to run and all invisible simcalls
 *  iteratively until there doesn't remain any. At this point, the function
 *  returns to the caller which can handle the visible (and ready) simcalls.
 */
XBT_PRIVATE void MC_wait_for_requests(void);

XBT_PRIVATE extern double *mc_time;

/** Execute a given simcall */
XBT_PRIVATE void MC_simcall_handle(smx_simcall_t req, int value);

SG_END_DECL()

#endif
