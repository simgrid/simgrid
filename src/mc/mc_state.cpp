/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_state.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"

#include <boost/range/algorithm.hpp>

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC (state)");

namespace simgrid {
namespace mc {

State::State(unsigned long state_number) : num(state_number)
{
  this->internal_comm.clear();
  std::memset(&this->internal_req, 0, sizeof(this->internal_req));
  std::memset(&this->executed_req, 0, sizeof(this->executed_req));

  actorStates.resize(MC_smx_get_maxpid());
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (state_number % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    system_state = std::make_shared<simgrid::mc::Snapshot>(num);
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
      MC_state_copy_incomplete_communications_pattern(this);
      MC_state_copy_index_communications_pattern(this);
    }
  }
}

std::size_t State::interleaveSize() const
{
  return boost::range::count_if(this->actorStates, [](simgrid::mc::ProcessState const& p) { return p.isTodo(); });
}

Transition State::getTransition() const
{
  return this->transition;
}

}
}

/* Search an enabled transition for the given process.
 *
 * This can be seen as an iterator returning the next transition of the process.
 *
 * We only consider the processes that are both
 *  - marked "to be interleaved" in their ProcessState (controlled by the checker algorithm).
 *  - which simcall can currently be executed (like a comm where the other partner is already known)
 * Once we returned the last enabled transition of a process, it is marked done.
 *
 * Things can get muddled with the WAITANY and TESTANY simcalls, that are rewritten on the fly to a bunch of WAIT
 * (resp TEST) transitions using the transition.argument field to remember what was the last returned sub-transition.
 */
static inline smx_simcall_t MC_state_get_request_for_process(simgrid::mc::State* state, smx_actor_t actor)
{
  /* reset the outgoing transition */
  simgrid::mc::ProcessState* procstate = &state->actorStates[actor->get_pid()];
  state->transition.pid                = -1;
  state->transition.argument           = -1;
  state->executed_req.call             = SIMCALL_NONE;

  if (not simgrid::mc::actor_is_enabled(actor))
    return nullptr; // Not executable in the application

  smx_simcall_t req = nullptr;
  switch (actor->simcall.call) {
    case SIMCALL_COMM_WAITANY:
      state->transition.argument = -1;
      while (procstate->times_considered < simcall_comm_waitany__get__count(&actor->simcall)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall, procstate->times_considered)) {
          state->transition.argument = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_waitany__get__count(&actor->simcall))
        procstate->setDone();
      if (state->transition.argument != -1)
        req = &actor->simcall;
      break;

    case SIMCALL_COMM_TESTANY: {
      unsigned start_count       = procstate->times_considered;
      state->transition.argument = -1;
      while (procstate->times_considered < simcall_comm_testany__get__count(&actor->simcall)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall, procstate->times_considered)) {
          state->transition.argument = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_testany__get__count(&actor->simcall))
        procstate->setDone();

      if (state->transition.argument != -1 || start_count == 0)
        req = &actor->simcall;

      break;
    }

    case SIMCALL_COMM_WAIT: {
      simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> remote_act =
          remote(static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(&actor->simcall)));
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_act;
      mc_model_checker->process().read(temp_act, remote_act);
      simgrid::kernel::activity::CommImpl* act = temp_act.get_buffer();
      if (act->src_actor_.get() && act->dst_actor_.get())
        state->transition.argument = 0;
      else if (act->src_actor_.get() == nullptr && act->type_ == simgrid::kernel::activity::CommImpl::Type::READY &&
               act->detached())
        state->transition.argument = 0;
      else
        state->transition.argument = -1;
      procstate->setDone();
      req = &actor->simcall;
      break;
    }

    case SIMCALL_MC_RANDOM: {
      int min_value              = simcall_mc_random__get__min(&actor->simcall);
      state->transition.argument = procstate->times_considered + min_value;
      procstate->times_considered++;
      if (state->transition.argument == simcall_mc_random__get__max(&actor->simcall))
        procstate->setDone();
      req = &actor->simcall;
      break;
    }

    default:
      procstate->setDone();
      state->transition.argument = 0;
      req                        = &actor->simcall;
      break;
  }
  if (not req)
    return nullptr;

  state->transition.pid = actor->get_pid();
  state->executed_req = *req;
  // Fetch the data of the request and translate it:
  state->internal_req = *req;

  /* The waitany and testany request are transformed into a wait or test request over the corresponding communication
   * action so it can be treated later by the dependence function. */
  switch (req->call) {
  case SIMCALL_COMM_WAITANY: {
    state->internal_req.call = SIMCALL_COMM_WAIT;
    simgrid::kernel::activity::CommImpl* remote_comm;
    remote_comm =
        mc_model_checker->process().read(remote(simcall_comm_waitany__get__comms(req) + state->transition.argument));
    mc_model_checker->process().read(state->internal_comm, remote(remote_comm));
    simcall_comm_wait__set__comm(&state->internal_req, state->internal_comm.get_buffer());
    simcall_comm_wait__set__timeout(&state->internal_req, 0);
    break;
  }

  case SIMCALL_COMM_TESTANY:
    state->internal_req.call = SIMCALL_COMM_TEST;

    if (state->transition.argument > 0) {
      simgrid::kernel::activity::CommImpl* remote_comm =
          mc_model_checker->process().read(remote(simcall_comm_testany__get__comms(req) + state->transition.argument));
      mc_model_checker->process().read(state->internal_comm, remote(remote_comm));
    }

    simcall_comm_test__set__comm(&state->internal_req, state->internal_comm.get_buffer());
    simcall_comm_test__set__result(&state->internal_req, state->transition.argument);
    break;

  case SIMCALL_COMM_WAIT:
    mc_model_checker->process().read_bytes(&state->internal_comm, sizeof(state->internal_comm),
                                           remote(simcall_comm_wait__getraw__comm(req)));
    simcall_comm_wait__set__comm(&state->executed_req, state->internal_comm.get_buffer());
    simcall_comm_wait__set__comm(&state->internal_req, state->internal_comm.get_buffer());
    break;

  case SIMCALL_COMM_TEST:
    mc_model_checker->process().read_bytes(&state->internal_comm, sizeof(state->internal_comm),
                                           remote(simcall_comm_test__getraw__comm(req)));
    simcall_comm_test__set__comm(&state->executed_req, state->internal_comm.get_buffer());
    simcall_comm_test__set__comm(&state->internal_req, state->internal_comm.get_buffer());
    break;

  default:
    /* No translation needed */
    break;
  }

  return req;
}

smx_simcall_t MC_state_get_request(simgrid::mc::State* state)
{
  for (auto& actor : mc_model_checker->process().actors()) {
    /* Only consider the actors that were marked as interleaving by the checker algorithm */
    if (not state->actorStates[actor.copy.get_buffer()->get_pid()].isTodo())
      continue;

    smx_simcall_t res = MC_state_get_request_for_process(state, actor.copy.get_buffer());
    if (res)
      return res;
  }
  return nullptr;
}
