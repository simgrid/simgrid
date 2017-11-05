/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid_config.h>

#include "mc/mc.h"
#include "src/mc/mc_base.h"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"

#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"

using simgrid::mc::remote;
#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(mc, "All MC categories");

int MC_random(int min, int max)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr);
#endif
  /* TODO, if the MC is disabled we do not really need to make a simcall for this :) */
  return simcall_mc_random(min, max);
}

namespace simgrid {
namespace mc {

void wait_for_requests()
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This must be called from the client");
#endif
  while (not simix_global->process_to_run.empty()) {
    SIMIX_process_runall();
    for (smx_actor_t const& process : simix_global->process_that_ran) {
      smx_simcall_t req = &process->simcall;
      if (req->call != SIMCALL_NONE && not simgrid::mc::request_is_visible(req))
        SIMIX_simcall_handle(req, 0);
    }
  }
#if SIMGRID_HAVE_MC
  xbt_dynar_reset(simix_global->actors_vector);
  for (std::pair<aid_t, smx_actor_t> const& kv : simix_global->process_list) {
    xbt_dynar_push_as(simix_global->actors_vector, smx_actor_t, kv.second);
  }
#endif
}

/** @brief returns if there this transition can proceed in a finite amount of time
 *
 * It is used in the model-checker to not get into self-deadlock where it would execute a never ending transition.
 *
 * Only WAIT operations (on comm, on mutex, etc) can ever return false because they could lock the MC exploration.
 * Wait operations are OK and return true in only two situations:
 *  - if the wait will succeed immediately (if both peer of the comm are there already or if the mutex is available)
 *  - if a timeout is provided, because we can fire the timeout if the transition is not ready without blocking in this
 * transition for ever.
 *
 */
// Called from both MCer and MCed:
bool actor_is_enabled(smx_actor_t actor)
{
#if SIMGRID_HAVE_MC
  // If in the MCer, ask the client app since it has all the data
  if (mc_model_checker != nullptr) {
    return mc_model_checker->process().actor_is_enabled(actor->pid);
  }
#endif

  // Now, we are in the client app, no need for remote memory reading.
  smx_simcall_t req = &actor->simcall;

  switch (req->call) {
    case SIMCALL_NONE:
      return false;

    case SIMCALL_COMM_WAIT: {
      /* FIXME: check also that src and dst processes are not suspended */
      simgrid::kernel::activity::CommImpl* act =
          static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(req));

      if (act->src_timeout || act->dst_timeout) {
        /* If it has a timeout it will be always be enabled (regardless of who declared the timeout),
         * because even if the communication is not ready, it can timeout and won't block. */
        if (_sg_mc_timeout == 1)
          return true;
      }
      /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
      else if (act->detached && act->src_proc == nullptr && act->type == SIMIX_COMM_READY)
        return (act->dst_proc != nullptr);
      return (act->src_proc && act->dst_proc);
    }

    case SIMCALL_COMM_WAITANY: {
      xbt_dynar_t comms = simcall_comm_waitany__get__comms(req);
      for (unsigned int index = 0; index < comms->used; ++index) {
        simgrid::kernel::activity::CommImpl* act = xbt_dynar_get_as(comms, index, simgrid::kernel::activity::CommImpl*);
        if (act->src_proc && act->dst_proc)
          return true;
      }
      return false;
    }

    case SIMCALL_MUTEX_LOCK: {
      smx_mutex_t mutex = simcall_mutex_lock__get__mutex(req);

      if (mutex->owner == nullptr)
        return true;
      return mutex->owner->pid == req->issuer->pid;
    }

    case SIMCALL_SEM_ACQUIRE: {
      static int warned = 0;
      if (not warned)
        XBT_INFO("Using semaphore in model-checked code is still experimental. Use at your own risk");
      warned = 1;
      return true;
    }

    case SIMCALL_COND_WAIT: {
      static int warned = 0;
      if (not warned)
        XBT_INFO("Using condition variables in model-checked code is still experimental. Use at your own risk");
      warned = 1;
      return true;
    }

    default:
      /* The rest of the requests are always enabled */
      return true;
  }
}

/* This is the list of requests that are visible from the checker algorithm.
 * Any other requests are handled right away on the application side.
 */
bool request_is_visible(smx_simcall_t req)
{
#if SIMGRID_HAVE_MC
  xbt_assert(mc_model_checker == nullptr, "This should be called from the client side");
#endif

  return req->call == SIMCALL_COMM_ISEND || req->call == SIMCALL_COMM_IRECV || req->call == SIMCALL_COMM_WAIT ||
         req->call == SIMCALL_COMM_WAITANY || req->call == SIMCALL_COMM_TEST || req->call == SIMCALL_COMM_TESTANY ||
         req->call == SIMCALL_MC_RANDOM || req->call == SIMCALL_MUTEX_LOCK || req->call == SIMCALL_MUTEX_TRYLOCK ||
         req->call == SIMCALL_MUTEX_UNLOCK;
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
  if (not MC_is_active() && MC_record_path.empty())
    return prng_random(min, max);
  return simcall->mc_value;
}
