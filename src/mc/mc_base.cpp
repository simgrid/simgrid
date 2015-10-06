/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <assert.h>

#include <simgrid/simix.h>

#include "mc_base.h"
#include "../simix/smx_private.h"
#include "mc/mc_record.h"
#include "mc/mc_replay.h"
#include "mc/mc.h"
#include "mc_protocol.h"

#ifdef HAVE_MC
#include "mc/Process.hpp"
#include "mc/ModelChecker.hpp"
#include "mc_smx.h"
#include "mc_server.h"
#endif

#ifdef HAVE_MC
using simgrid::mc::remote;
#endif

extern "C" {

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");

int MC_random(int min, int max)
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  /* TODO, if the MC is disabled we do not really need to make a simcall for
   * this :) */
  /* FIXME: return mc_current_state->executed_transition->random.value; */
  return simcall_mc_random(min, max);
}

void MC_wait_for_requests(void)
{
#ifdef HAVE_MC
  if (mc_mode == MC_MODE_SERVER) {
    MC_server_wait_client(&mc_model_checker->process());
    return;
  }
#endif

  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !MC_request_is_visible(req))
        SIMIX_simcall_handle(req, 0);
    }
  }
}

// Called from both MCer and MCed:
int MC_request_is_enabled(smx_simcall_t req)
{
  unsigned int index = 0;
  smx_synchro_t act = 0;
#ifdef HAVE_MC
  s_smx_synchro_t temp_synchro;
#endif

  switch (req->call) {
  case SIMCALL_NONE:
    return FALSE;

  case SIMCALL_COMM_WAIT:
    /* FIXME: check also that src and dst processes are not suspended */
    act = simcall_comm_wait__get__comm(req);

#ifdef HAVE_MC
    // Fetch from MCed memory:
    if (mc_mode == MC_MODE_SERVER) {
      mc_model_checker->process().read(&temp_synchro, remote(act));
      act = &temp_synchro;
    }
#endif

    if (simcall_comm_wait__get__timeout(req) >= 0) {
      /* If it has a timeout it will be always be enabled, because even if the
       * communication is not ready, it can timeout and won't block. */
      if (_sg_mc_timeout == 1)
        return TRUE;
    } else {
      /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
      if (act->comm.detached && act->comm.src_proc == NULL
          && act->comm.type == SIMIX_COMM_READY)
        return (act->comm.dst_proc != NULL);
    }
    return (act->comm.src_proc && act->comm.dst_proc);

  case SIMCALL_COMM_WAITANY: {
    xbt_dynar_t comms;
#ifdef HAVE_MC

    s_xbt_dynar_t comms_buffer;
    size_t buffer_size = 0;
    if (mc_mode == MC_MODE_SERVER) {
      // Read dynar:
      mc_model_checker->process().read(
        &comms_buffer, remote(simcall_comm_waitany__get__comms(req)));
      assert(comms_buffer.elmsize == sizeof(act));
      buffer_size = comms_buffer.elmsize * comms_buffer.used;
      comms = &comms_buffer;
    } else {
      comms = simcall_comm_waitany__get__comms(req);
    }
    // Read all the dynar buffer:
    char buffer[buffer_size];
    if (mc_mode == MC_MODE_SERVER)
      mc_model_checker->process().read_bytes(buffer, sizeof(buffer),
        remote(comms->data));
#else
    comms = simcall_comm_waitany__get__comms(req);
#endif

    for (index = 0; index < comms->used; ++index) {
#ifdef HAVE_MC
      // Fetch act from MCed memory:
      if (mc_mode == MC_MODE_SERVER) {
        memcpy(&act, buffer + comms->elmsize * index, sizeof(act));
        mc_model_checker->process().read(&temp_synchro, remote(act));
        act = &temp_synchro;
      }
      else
#endif
        act = xbt_dynar_get_as(comms, index, smx_synchro_t);
      if (act->comm.src_proc && act->comm.dst_proc)
        return TRUE;
    }
    return FALSE;
  }

  case SIMCALL_MUTEX_TRYLOCK:
    return TRUE;

  case SIMCALL_MUTEX_LOCK: {
    smx_mutex_t mutex = simcall_mutex_lock__get__mutex(req);
#ifdef HAVE_MC
    s_smx_mutex_t temp_mutex;
    if (mc_mode == MC_MODE_SERVER) {
      mc_model_checker->process().read(&temp_mutex, remote(mutex));
      mutex = &temp_mutex;
    }
#endif
    if(mutex->owner == NULL)
      return TRUE;
    else
#ifdef HAVE_MC
      // TODO, *(mutex->owner) :/
      return MC_smx_resolve_process(mutex->owner)->pid ==
        MC_smx_resolve_process(req->issuer)->pid;
#else
      return mutex->owner->pid == req->issuer->pid;
#endif
    }

  default:
    /* The rest of the requests are always enabled */
    return TRUE;
  }
}

int MC_request_is_visible(smx_simcall_t req)
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
#ifdef HAVE_MC
      || req->call == SIMCALL_MC_SNAPSHOT
      || req->call == SIMCALL_MC_COMPARE_SNAPSHOTS
#endif
      ;
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
#ifndef _XBT_WIN32
    x = (unsigned long) random();
#else
    x = (unsigned long) rand();
#endif
  } while( x >= accept_size );
  return min + (x % output_size);
}

int simcall_HANDLER_mc_random(smx_simcall_t simcall, int min, int max)
{
  if (!MC_is_active() && !MC_record_path){
    return prng_random(min, max);
  }

  return simcall->mc_value;
}

void MC_simcall_handle(smx_simcall_t req, int value)
{
#ifndef HAVE_MC
  SIMIX_simcall_handle(req, value);
#else
  if (mc_mode == MC_MODE_CLIENT) {
    SIMIX_simcall_handle(req, value);
    return;
  }

  unsigned i;
  mc_smx_process_info_t pi = NULL;

  xbt_dynar_foreach_ptr(mc_model_checker->process().smx_process_infos, i, pi) {
    if (req == &pi->copy.simcall) {
      MC_server_simcall_handle(&mc_model_checker->process(), pi->copy.pid, value);
      return;
    }
  }

  xbt_die("Could not find the request");
#endif
}

}
