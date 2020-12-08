/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REQUEST_HPP
#define SIMGRID_MC_REQUEST_HPP

#include "src/simix/smx_private.hpp"

namespace simgrid {
namespace mc {

enum class RequestType {
  simix,
  executed,
  internal,
};

XBT_PRIVATE bool request_depend(smx_simcall_t req1, smx_simcall_t req2);

XBT_PRIVATE bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx);
}
}

#endif
