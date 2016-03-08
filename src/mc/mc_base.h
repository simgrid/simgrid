/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASE_H
#define SIMGRID_MC_BASE_H

#include <xbt/base.h>
#include "src/simix/popping_private.h" // smx_simcall_t

#ifdef __cplusplus

namespace simgrid {
namespace mc {

/** Can this requests can be executed.
 *
 *  Most requests are always enabled but WAIT and WAITANY
 *  are not always enabled: a WAIT where the communication does not
 *  have both a source and a destination yet is not enabled
 *  (unless timeout is enabled in the wait and enabeld in SimGridMC).
 */
XBT_PRIVATE bool request_is_enabled(smx_simcall_t req);

/** Execute everything which is invisible
 *
 *  Execute all the processes that are ready to run and all invisible simcalls
 *  iteratively until there doesn't remain any. At this point, the function
 *  returns to the caller which can handle the visible (and ready) simcalls.
 */
XBT_PRIVATE void wait_for_requests(void);

XBT_PRIVATE extern std::vector<double> processes_time;

/** Execute a given simcall */
XBT_PRIVATE void handle_simcall(smx_simcall_t req, int value);

/** Check if the given simcall is visible
 *
 *  \return `TRUE` or `FALSE`
 */
XBT_PRIVATE bool request_is_visible(smx_simcall_t req);

}
}

#endif

#endif
