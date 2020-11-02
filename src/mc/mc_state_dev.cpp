/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/mc_state_dev.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_smx.hpp"

#include <boost/range/algorithm.hpp>

using simgrid::mc::remote;

namespace simgrid {
namespace mc {

State_Dev::State_Dev(unsigned long state_number) : num_(state_number)
{
  actor_states_.resize(MC_smx_get_maxpid());
}

std::size_t State_Dev::interleave_size() const
{
  return boost::range::count_if(this->actor_states_, [](simgrid::mc::ActorState_Dev const& a) { return a.is_todo(); });
}

}
}

/* Search an enabled transition for the given process.
 *
 * This can be seen as an iterator returning the next transition of the process.
 *
 * We only consider the processes that are both
 *  - marked "to be interleaved" in their ActorState (controlled by the checker algorithm).
 *  - which simcall can currently be executed (like a comm where the other partner is already known)
 * Once we returned the last enabled transition of a process, it is marked done.
 *
 * Things can get muddled with the WAITANY and TESTANY simcalls, that are rewritten on the fly to a bunch of WAIT
 * (resp TEST) transitions using the transition.argument field to remember what was the last returned sub-transition.
 */
static inline smx_simcall_t MC_state_choose_request_for_process_dev(simgrid::mc::State_Dev* state, smx_actor_t actor)
{
  /* reset the outgoing transition */
  simgrid::mc::ActorState_Dev* procstate   = &state->actor_states_[actor->get_pid()];
  state->transition_.pid_              = -1;
  state->transition_.argument_         = -1;
  state->executed_req_.call_           = SIMCALL_NONE;

  if (not simgrid::mc::actor_is_enabled(actor))
    return nullptr; // Not executable in the application

  smx_simcall_t req = nullptr;
  switch (actor->simcall_.call_) {
    case SIMCALL_COMM_WAITANY:
      state->transition_.argument_ = -1;
      while (procstate->times_considered < simcall_comm_waitany__get__count(&actor->simcall_)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
          state->transition_.argument_ = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_waitany__get__count(&actor->simcall_))
        procstate->set_done();
      if (state->transition_.argument_ != -1)
        req = &actor->simcall_;
      break;

    case SIMCALL_COMM_TESTANY: {
      unsigned start_count       = procstate->times_considered;
      state->transition_.argument_ = -1;
      while (procstate->times_considered < simcall_comm_testany__get__count(&actor->simcall_)) {
        if (simgrid::mc::request_is_enabled_by_idx(&actor->simcall_, procstate->times_considered)) {
          state->transition_.argument_ = procstate->times_considered;
          ++procstate->times_considered;
          break;
        }
        ++procstate->times_considered;
      }

      if (procstate->times_considered >= simcall_comm_testany__get__count(&actor->simcall_))
        procstate->set_done();

      if (state->transition_.argument_ != -1 || start_count == 0)
        req = &actor->simcall_;

      break;
    }

    case SIMCALL_COMM_WAIT: {
      simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> remote_act =
          remote(simcall_comm_wait__getraw__comm(&actor->simcall_));
      simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> temp_act;
      mc_model_checker->get_remote_simulation().read(temp_act, remote_act);
      const simgrid::kernel::activity::CommImpl* act = temp_act.get_buffer();
      if (act->src_actor_.get() && act->dst_actor_.get())
        state->transition_.argument_ = 0; // OK
      else if (act->src_actor_.get() == nullptr && act->type_ == simgrid::kernel::activity::CommImpl::Type::READY &&
               act->detached())
        state->transition_.argument_ = 0; // OK
      else
        state->transition_.argument_ = -1; // timeout
      procstate->set_done();
      req = &actor->simcall_;
      break;
    }

    case SIMCALL_MC_RANDOM: {
      int min_value                = simcall_mc_random__get__min(&actor->simcall_);
      state->transition_.argument_ = procstate->times_considered + min_value;
      procstate->times_considered++;
      if (state->transition_.argument_ == simcall_mc_random__get__max(&actor->simcall_))
        procstate->set_done();
      req = &actor->simcall_;
      break;
    }

    default:
      procstate->set_done();
      state->transition_.argument_ = 0;
      req                          = &actor->simcall_;
      break;
  }
  if (not req)
    return nullptr;

  state->transition_.pid_ = actor->get_pid();
  state->executed_req_    = *req;
  // Fetch the data of the request and translate it:
  state->internal_req_ = *req;

  /* The waitany and testany request are transformed into a wait or test request over the corresponding communication
   * action so it can be treated later by the dependence function. */
  switch (req->call_) {
    case SIMCALL_COMM_WAITANY: {
      // state->internal_req_.call_ = SIMCALL_COMM_WAIT;
      // simgrid::kernel::activity::CommImpl* remote_comm;
      // remote_comm = mc_model_checker->get_remote_simulation().read(
      //     remote(simcall_comm_waitany__get__comms(req) + state->transition_.argument_));
      // mc_model_checker->get_remote_simulation().read(state->internal_comm_, remote(remote_comm));
      // simcall_comm_wait__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      // simcall_comm_wait__set__timeout(&state->internal_req_, 0);
      break;
    }

    case SIMCALL_COMM_TESTANY:
      // state->internal_req_.call_ = SIMCALL_COMM_TEST;

      // // if (state->transition_.argument_ > 0) {
      // //   simgrid::kernel::activity::CommImpl* remote_comm = mc_model_checker->get_remote_simulation().read(
      // //       remote(simcall_comm_testany__get__comms(req) + state->transition_.argument_));
      // //   mc_model_checker->get_remote_simulation().read(state->internal_comm_, remote(remote_comm));
      // // }

      // simcall_comm_test__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      // simcall_comm_test__set__result(&state->internal_req_, state->transition_.argument_);
      break;

    case SIMCALL_COMM_WAIT:
      // mc_model_checker->get_remote_simulation().read_bytes(&state->internal_comm_, sizeof(state->internal_comm_),
      //                                                      remote(simcall_comm_wait__getraw__comm(req)));
      // simcall_comm_wait__set__comm(&state->executed_req_, state->internal_comm_.get_buffer());
      // simcall_comm_wait__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      break;

    case SIMCALL_COMM_TEST:
      // mc_model_checker->get_remote_simulation().read_bytes(&state->internal_comm_, sizeof(state->internal_comm_),
      //                                                      remote(simcall_comm_test__getraw__comm(req)));
      // simcall_comm_test__set__comm(&state->executed_req_, state->internal_comm_.get_buffer());
      // simcall_comm_test__set__comm(&state->internal_req_, state->internal_comm_.get_buffer());
      break;

    default:
      /* No translation needed */
      break;
  }

  return req;
}

smx_simcall_t MC_state_choose_request_dev(simgrid::mc::State_Dev* state)
{
  for (auto& actor : mc_model_checker->get_remote_simulation().actors()) {
    /* Only consider the actors that were marked as interleaving by the checker algorithm */
    if (not state->actor_states_[actor.copy.get_buffer()->get_pid()].is_todo())
      continue;

    smx_simcall_t res = MC_state_choose_request_for_process_dev(state, actor.copy.get_buffer());
    if (res)
      return res;
  }
  return nullptr;
}
