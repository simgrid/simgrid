/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid_config.h>

#include "mc/mc.h"
#include "src/mc/mc_base.h"
#include "src/mc/mc_replay.h"
#include "src/simix/smx_private.h"

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
  xbt_assert(mc_model_checker == nullptr);
#endif

  smx_actor_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (not xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && not simgrid::mc::request_is_visible(req))
        SIMIX_simcall_handle(req, 0);
    }
  }
#if SIMGRID_HAVE_MC
  xbt_dynar_reset(simix_global->actors_vector);
  for (std::pair<aid_t, smx_actor_t> kv : simix_global->process_list) {
    xbt_dynar_push_as(simix_global->actors_vector, smx_actor_t, kv.second);
  }
#endif
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
  if (not MC_is_active() && not MC_record_path)
    return prng_random(min, max);
  return simcall->mc_value;
}
