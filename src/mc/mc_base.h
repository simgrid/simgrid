/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_BASE_H
#define SIMGRID_MC_BASE_H

#include "simgrid/forward.h"

#ifdef __cplusplus

#include <vector>

namespace simgrid {
namespace mc {

/** Execute everything which is invisible
 *
 *  Execute all the processes that are ready to run and all invisible simcalls
 *  iteratively until there doesn't remain any. At this point, the function
 *  returns to the caller which can handle the visible (and ready) simcalls.
 */
XBT_PRIVATE void wait_for_requests();

XBT_PRIVATE extern std::vector<double> processes_time;

/** Execute a given simcall */
XBT_PRIVATE void handle_simcall(smx_simcall_t req, int req_num);

/** Is the process ready to execute its simcall?
 *
 *  This is true if the request associated with the process is ready.
 *
 *  Most requests are always enabled but WAIT and WAITANY
 *  are not always enabled: a WAIT where the communication does not
 *  have both a source and a destination yet is not enabled
 *  (unless timeout is enabled in the wait and enabled in SimGridMC).
 */
XBT_PRIVATE bool actor_is_enabled(smx_actor_t process);

/** Check if the given simcall is visible */
XBT_PRIVATE bool request_is_visible(smx_simcall_t req);

}
}

#endif

#endif
