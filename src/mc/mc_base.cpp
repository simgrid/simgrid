/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include <algorithm>

#include <simgrid_config.h>

#include <xbt/log.h>
#include <xbt/asserts.h>
#include <xbt/dynar.h>

#include <simgrid/simix.h>

#include "src/mc/mc_base.h"
#include "src/simix/smx_private.h"
#include "src/mc/mc_replay.h"
#include "mc/mc.h"
#include "src/mc/mc_protocol.h"

#include "src/simix/Synchro.h"
#include "src/simix/SynchroIo.hpp"
#include "src/simix/SynchroComm.hpp"
#include "src/simix/SynchroRaw.hpp"
#include "src/simix/SynchroSleep.hpp"
#include "src/simix/SynchroExec.hpp"

#if HAVE_MC
#include "src/mc/mc_request.h"
#include "src/mc/Process.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_smx.h"
#endif

#if HAVE_MC
using simgrid::mc::remote;
#endif

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");

int MC_random(int min, int max)
{
#if HAVE_MC
  xbt_assert(mc_model_checker == nullptr);
  /* TODO, if the MC is disabled we do not really need to make a simcall for
   * this :) */
#endif
  return simcall_mc_random(min, max);
}

namespace simgrid {
namespace mc {

void wait_for_requests(void)
{
#if HAVE_MC
  xbt_assert(mc_model_checker == nullptr);
#endif

  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !simgrid::mc::request_is_visible(req))
        SIMIX_simcall_handle(req, 0);
    }
  }
}

// Called from both MCer and MCed:
bool request_is_enabled(smx_simcall_t req)
{
  unsigned int index = 0;
  // TODO, add support for the subtypes?

  switch (req->call) {
  case SIMCALL_NONE:
    return false;

  case SIMCALL_COMM_WAIT:
  {
    /* FIXME: check also that src and dst processes are not suspended */
    simgrid::simix::Comm *act = static_cast<simgrid::simix::Comm*>(simcall_comm_wait__get__comm(req));

#if HAVE_MC
    // Fetch from MCed memory:
    // HACK, type puning
    simgrid::mc::Remote<simgrid::simix::Comm> temp_comm;
    if (mc_model_checker != nullptr) {
      mc_model_checker->process().read(temp_comm, remote(act));
      act = static_cast<simgrid::simix::Comm*>(temp_comm.data());
    }
#endif

    if (simcall_comm_wait__get__timeout(req) >= 0) {
      /* If it has a timeout it will be always be enabled, because even if the
       * communication is not ready, it can timeout and won't block. */
      if (_sg_mc_timeout == 1)
        return true;
    }
    /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
    else if (act->detached && act->src_proc == nullptr
          && act->type == SIMIX_COMM_READY)
        return (act->dst_proc != nullptr);
    return (act->src_proc && act->dst_proc);
  }

  case SIMCALL_COMM_WAITANY: {
    xbt_dynar_t comms;
    simgrid::simix::Comm *act = static_cast<simgrid::simix::Comm*>(simcall_comm_wait__get__comm(req));
#if HAVE_MC

    s_xbt_dynar_t comms_buffer;
    size_t buffer_size = 0;
    if (mc_model_checker != nullptr) {
      // Read dynar:
      mc_model_checker->process().read(
        &comms_buffer, remote(simcall_comm_waitany__get__comms(req)));
      assert(comms_buffer.elmsize == sizeof(act));
      buffer_size = comms_buffer.elmsize * comms_buffer.used;
      comms = &comms_buffer;
    } else
      comms = simcall_comm_waitany__get__comms(req);

    // Read all the dynar buffer:
    char buffer[buffer_size];
    if (mc_model_checker != nullptr)
      mc_model_checker->process().read_bytes(buffer, sizeof(buffer),
        remote(comms->data));
#else
    comms = simcall_comm_waitany__get__comms(req);
#endif

    for (index = 0; index < comms->used; ++index) {
#if HAVE_MC
      // Fetch act from MCed memory:
      // HACK, type puning
      simgrid::mc::Remote<simgrid::simix::Comm> temp_comm;
      if (mc_model_checker != nullptr) {
        memcpy(&act, buffer + comms->elmsize * index, sizeof(act));
        mc_model_checker->process().read(temp_comm, remote(act));
        act = static_cast<simgrid::simix::Comm*>(temp_comm.data());
      }
      else
#endif
        act = xbt_dynar_get_as(comms, index, simgrid::simix::Comm*);
      if (act->src_proc && act->dst_proc)
        return true;
    }
    return false;
  }

  case SIMCALL_MUTEX_TRYLOCK:
    return true;

  case SIMCALL_MUTEX_LOCK: {
    smx_mutex_t mutex = simcall_mutex_lock__get__mutex(req);
#if HAVE_MC
    s_smx_mutex_t temp_mutex;
    if (mc_model_checker != nullptr) {
      mc_model_checker->process().read(&temp_mutex, remote(mutex));
      mutex = &temp_mutex;
    }
#endif

    if(mutex->owner == nullptr)
      return true;
#if HAVE_MC
    else if (mc_model_checker != nullptr) {
      simgrid::mc::Process& modelchecked = mc_model_checker->process();
      // TODO, *(mutex->owner) :/
      return modelchecked.resolveProcess(simgrid::mc::remote(mutex->owner))->pid
        == modelchecked.resolveProcess(simgrid::mc::remote(req->issuer))->pid;
    }
#endif
    else
      return mutex->owner->pid == req->issuer->pid;
    }

  default:
    /* The rest of the requests are always enabled */
    return true;
  }
}

bool request_is_visible(smx_simcall_t req)
{
  return req->call == SIMCALL_COMM_ISEND
      || req->call == SIMCALL_COMM_IRECV
      || req->call == SIMCALL_COMM_WAIT
      || req->call == SIMCALL_COMM_WAITANY
      || req->call == SIMCALL_COMM_TEST
      || req->call == SIMCALL_COMM_TESTANY
      || req->call == SIMCALL_MC_RANDOM
      || req->call == SIMCALL_MUTEX_LOCK
      || req->call == SIMCALL_MUTEX_TRYLOCK
      ;
}

}
}

static int prng_random(int min, int max)
{
  unsigned long output_size = ((unsigned long) max - (unsigned long) min) + 1;
  unsigned long input_size = (unsigned long) RAND_MAX + 1;
  unsigned long reject_size = input_size % output_size;
  unsigned long accept_size = input_size - reject_size; // module*accept_size

  // Use rejection in order to avoid skew
  unsigned long x;
  do {
#ifndef _WIN32
    x = (unsigned long) random();
#else
    x = (unsigned long) rand();
#endif
  } while( x >= accept_size );
  return min + (x % output_size);
}

int simcall_HANDLER_mc_random(smx_simcall_t simcall, int min, int max)
{
  if (!MC_is_active() && !MC_record_path)
    return prng_random(min, max);
  return simcall->mc_value;
}
