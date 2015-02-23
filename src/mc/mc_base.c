/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/simix.h>

#include "mc_base.h"
#include "../simix/smx_private.h"
#include "mc_record.h"

#ifdef HAVE_MC
#include "mc_process.h"
#include "mc_model_checker.h"
#include "mc_protocol.h"
#endif

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");

void MC_wait_for_requests(void)
{
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
    if (!MC_process_is_self(&mc_model_checker->process)) {
      MC_process_read(&mc_model_checker->process, MC_PROCESS_NO_FLAG,
        &temp_synchro, act, sizeof(temp_synchro),
        MC_PROCESS_INDEX_ANY);
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

  case SIMCALL_COMM_WAITANY:
    /* Check if it has at least one communication ready */
    xbt_dynar_foreach(simcall_comm_waitany__get__comms(req), index, act) {

#ifdef HAVE_MC
      // Fetch from MCed memory:
      if (!MC_process_is_self(&mc_model_checker->process)) {
        MC_process_read(&mc_model_checker->process, MC_PROCESS_NO_FLAG,
          &temp_synchro, act, sizeof(temp_synchro),
          MC_PROCESS_INDEX_ANY);
        act = &temp_synchro;
      }
#endif

      if (act->comm.src_proc && act->comm.dst_proc)
        return TRUE;
    }
    return FALSE;

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
#ifdef HAVE_MC
      || req->call == SIMCALL_MC_SNAPSHOT
      || req->call == SIMCALL_MC_COMPARE_SNAPSHOTS
#endif
      ;
}

int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value; */
  return simcall_mc_random(min, max);
}

static int prng_random(int min, int max)
{
  unsigned long output_size = ((unsigned long) max - (unsigned long) min) + 1;
  unsigned long input_size = (unsigned long) RAND_MAX + 1;
  unsigned long reject_size = input_size % output_size;
  unsigned long accept_size = input_size - reject_size; // module*accept_size

  // Use rejection in order to avoid skew
  long x;
  do {
#ifndef _XBT_WIN32
    x = random();
#else
    x = rand();
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
