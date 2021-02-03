/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_request.hpp"
#include "src/include/mc/mc.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/checker/SimcallInspector.hpp"
#include "src/mc/mc_smx.hpp"
#include <array>

using simgrid::mc::remote;
using simgrid::simix::Simcall;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc, "Logging specific to MC (request)");

namespace simgrid {
namespace mc {

bool request_is_enabled_by_idx(smx_simcall_t req, unsigned int idx)
{
  kernel::activity::CommImpl* remote_act = nullptr;
  switch (req->call_) {
    case Simcall::COMM_WAIT:
      /* FIXME: check also that src and dst processes are not suspended */
      remote_act = simcall_comm_wait__getraw__comm(req);
      break;

    case Simcall::COMM_WAITANY:
      remote_act = mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__get__comms(req) + idx));
      break;

    case Simcall::COMM_TESTANY:
      remote_act = mc_model_checker->get_remote_simulation().read(remote(simcall_comm_testany__get__comms(req) + idx));
      break;

    default:
      return true;
  }

  Remote<kernel::activity::CommImpl> temp_comm;
  mc_model_checker->get_remote_simulation().read(temp_comm, remote(remote_act));
  const kernel::activity::CommImpl* comm = temp_comm.get_buffer();
  return comm->src_actor_.get() && comm->dst_actor_.get();
}

} // namespace mc
} // namespace simgrid
