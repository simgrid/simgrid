/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REQUEST_H
#define SIMGRID_MC_REQUEST_H

#include "src/simix/smx_private.h"

namespace simgrid {
namespace mc {

enum class RequestType {
  simix,
  executed,
  internal,
};

XBT_PRIVATE bool request_depend(smx_simcall_t req1, smx_simcall_t req2);

XBT_PRIVATE std::string request_to_string(smx_simcall_t req, int value, simgrid::mc::RequestType type);

XBT_PRIVATE bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);

/** Is the process ready to execute its simcall?
 *
 *  This is true if the request associated with the process is ready.
 *
 *  Most requests are always enabled but WAIT and WAITANY
 *  are not always enabled: a WAIT where the communication does not
 *  have both a source and a destination yet is not enabled
 *  (unless timeout is enabled in the wait and enabeld in SimGridMC).
 */
XBT_PRIVATE bool actor_is_enabled(smx_actor_t process);

XBT_PRIVATE std::string request_get_dot_output(smx_simcall_t req, int value);

}
}

#endif
