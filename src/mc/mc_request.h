/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REQUEST_H
#define SIMGRID_MC_REQUEST_H

#include <xbt/base.h>

#include "src/simix/smx_private.h"

namespace simgrid {
namespace mc {

enum class RequestType {
  simix,
  executed,
  internal,
};

XBT_PRIVATE bool request_depend(smx_simcall_t req1, smx_simcall_t req2);

XBT_PRIVATE char* request_to_string(smx_simcall_t req, int value, simgrid::mc::RequestType type);

XBT_PRIVATE bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);

/** Is the process ready to execute its simcall?
 *
 *  This is true if the request associated with the process is ready.
 */
XBT_PRIVATE bool process_is_enabled(smx_process_t process);

XBT_PRIVATE char *request_get_dot_output(smx_simcall_t req, int value);

}
}

#endif
