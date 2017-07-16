/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

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

/** Check if the given simcall is visible
 *
 *  \return `TRUE` or `FALSE`
 */
XBT_PRIVATE bool request_is_visible(smx_simcall_t req);

}
}

#endif

#endif
