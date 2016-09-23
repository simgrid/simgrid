/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <assert.h>

#include <boost/range/algorithm.hpp>

#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/simix/smx_private.h"
#include "src/mc/mc_state.h"
#include "src/mc/mc_request.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_xbt.hpp"
#include "src/mc/Transition.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc,
                                "Logging specific to MC (state)");

/**
 * \brief Creates a state data structure used by the exploration algorithm
 */
simgrid::mc::State* MC_state_new(unsigned long state_number)
{
  simgrid::mc::State* state = new simgrid::mc::State();
  state->processStates.resize(MC_smx_get_maxpid());
  state->num = state_number;
  /* Stateful model checking */
  if((_sg_mc_checkpoint > 0 && (state_number % _sg_mc_checkpoint == 0)) ||  _sg_mc_termination){
    state->system_state = simgrid::mc::take_snapshot(state->num);
    if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
      MC_state_copy_incomplete_communications_pattern(state);
      MC_state_copy_index_communications_pattern(state);
    }
  }
  return state;
}

namespace simgrid {
namespace mc {

State::State()
{
  this->internal_comm.clear();
  std::memset(&this->internal_req, 0, sizeof(this->internal_req));
  std::memset(&this->executed_req, 0, sizeof(this->executed_req));
}

std::size_t State::interleaveSize() const
{
  return boost::range::count_if(this->processStates,
    [](simgrid::mc::ProcessState const& p) { return p.isToInterleave(); });
}

Transition State::getTransition() const
{
  return this->transition;
}

}
}

static inline smx_simcall_t MC_state_get_request_for_process(
  simgrid::mc::State* state, smx_actor_t process)
{
  simgrid::mc::ProcessState* procstate = &state->processStates[process->pid];
  state->transition.pid = -1;
  state->transition.argument = -1;

  if (!procstate->isToInterleave())
    return nullptr;
  if (!simgrid::mc::process_is_enabled(process))
    return nullptr;

  smx_simcall_t req = nullptr;
  switch (process->simcall.call) {
      case SIMCALL_COMM_WAITANY:
        state->transition.argument = -1;
        while (procstate->interleave_count <
              read_length(mc_model_checker->process(),
                remote(simcall_comm_waitany__get__comms(&process->simcall)))) {
          if (simgrid::mc::request_is_enabled_by_idx(&process->simcall,
              procstate->interleave_count++)) {
            state->transition.argument = procstate->interleave_count - 1;
            break;
          }
        }

        if (procstate->interleave_count >=
            simgrid::mc::read_length(mc_model_checker->process(),
              simgrid::mc::remote(simcall_comm_waitany__get__comms(&process->simcall))))
          procstate->setDone();
        if (state->transition.argument != -1)
          req = &process->simcall;
        break;

      case SIMCALL_COMM_TESTANY: {
        unsigned start_count = procstate->interleave_count;
        state->transition.argument = -1;
        while (procstate->interleave_count <
                simcall_comm_testany__get__count(&process->simcall))
          if (simgrid::mc::request_is_enabled_by_idx(&process->simcall,
              procstate->interleave_count++)) {
            state->transition.argument = procstate->interleave_count - 1;
            break;
          }

        if (procstate->interleave_count >=
            simcall_comm_testany__get__count(&process->simcall))
          procstate->setDone();

        if (state->transition.argument != -1 || start_count == 0)
           req = &process->simcall;

        break;
      }

      case SIMCALL_COMM_WAIT: {
        simgrid::mc::RemotePtr<simgrid::kernel::activity::Comm> remote_act = remote(
          static_cast<simgrid::kernel::activity::Comm*>(simcall_comm_wait__get__comm(&process->simcall)));
        simgrid::mc::Remote<simgrid::kernel::activity::Comm> temp_act;
        mc_model_checker->process().read(temp_act, remote_act);
        simgrid::kernel::activity::Comm* act = temp_act.getBuffer();
        if (act->src_proc && act->dst_proc)
          state->transition.argument = 0;
        else if (act->src_proc == nullptr && act->type == SIMIX_COMM_READY
              && act->detached == 1)
          state->transition.argument = 0;
        else
          state->transition.argument = -1;
        procstate->setDone();
        req = &process->simcall;
        break;
      }

      case SIMCALL_MC_RANDOM: {
        int min_value = simcall_mc_random__get__min(&process->simcall);
        state->transition.argument = procstate->interleave_count + min_value;
        procstate->interleave_count++;
        if (state->transition.argument == simcall_mc_random__get__max(&process->simcall))
          procstate->setDone();
        req = &process->simcall;
        break;
      }

      default:
        procstate->setDone();
        state->transition.argument = 0;
        req = &process->simcall;
        break;
  }
  if (!req)
    return nullptr;

  // Fetch the data of the request and translate it:

  state->transition.pid = process->pid;

  state->executed_req = *req;

  /* The waitany and testany request are transformed into a wait or test request
   * over the corresponding communication action so it can be treated later by
   * the dependence function. */
  switch (req->call) {
  case SIMCALL_COMM_WAITANY: {
    state->internal_req.call = SIMCALL_COMM_WAIT;
    state->internal_req.issuer = req->issuer;
    smx_activity_t remote_comm;
    read_element(mc_model_checker->process(),
      &remote_comm, remote(simcall_comm_waitany__get__comms(req)),
      state->transition.argument, sizeof(remote_comm));
    mc_model_checker->process().read(state->internal_comm, remote(
      static_cast<simgrid::kernel::activity::Comm*>(remote_comm)));
    simcall_comm_wait__set__comm(&state->internal_req, state->internal_comm.getBuffer());
    simcall_comm_wait__set__timeout(&state->internal_req, 0);
    break;
  }

  case SIMCALL_COMM_TESTANY:
    state->internal_req.call = SIMCALL_COMM_TEST;
    state->internal_req.issuer = req->issuer;

    if (state->transition.argument > 0) {
      smx_activity_t remote_comm = mc_model_checker->process().read(
        remote(simcall_comm_testany__get__comms(req) + state->transition.argument));
      mc_model_checker->process().read(state->internal_comm, remote(
        static_cast<simgrid::kernel::activity::Comm*>(remote_comm)));
    }

    simcall_comm_test__set__comm(&state->internal_req, state->internal_comm.getBuffer());
    simcall_comm_test__set__result(&state->internal_req, state->transition.argument);
    break;

  case SIMCALL_COMM_WAIT:
    state->internal_req = *req;
    mc_model_checker->process().read_bytes(&state->internal_comm ,
      sizeof(state->internal_comm), remote(simcall_comm_wait__get__comm(req)));
    simcall_comm_wait__set__comm(&state->executed_req, state->internal_comm.getBuffer());
    simcall_comm_wait__set__comm(&state->internal_req, state->internal_comm.getBuffer());
    break;

  case SIMCALL_COMM_TEST:
    state->internal_req = *req;
    mc_model_checker->process().read_bytes(&state->internal_comm,
      sizeof(state->internal_comm), remote(simcall_comm_test__get__comm(req)));
    simcall_comm_test__set__comm(&state->executed_req, state->internal_comm.getBuffer());
    simcall_comm_test__set__comm(&state->internal_req, state->internal_comm.getBuffer());
    break;

  default:
    state->internal_req = *req;
    break;
  }

  return req;
}

smx_simcall_t MC_state_get_request(simgrid::mc::State* state)
{
  for (auto& p : mc_model_checker->process().simix_processes()) {
    smx_simcall_t res = MC_state_get_request_for_process(state, p.copy.getBuffer());
    if (res)
      return res;
  }
  return nullptr;
}
