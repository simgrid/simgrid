/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Transition.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/mc_state.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;

std::string Transition::to_string() const
{
  xbt_assert(mc_model_checker != nullptr, "Must be called from MCer");

  return textual_;
}
RemotePtr<simgrid::kernel::actor::SimcallObserver> Transition::execute(simgrid::mc::State* state, int next)
{
  std::vector<ActorInformation>& actors = mc_model_checker->get_remote_process().actors();

  kernel::actor::ActorImpl* actor = actors[next].copy.get_buffer();
  aid_t aid                       = actor->get_pid();
  int times_considered;

  simgrid::mc::ActorState* actor_state = &state->actor_states_[aid];
  /* This actor is ready to be executed. Prepare its execution when simcall_handle will be called on it */
  if (actor->simcall_.observer_ != nullptr) {
    times_considered = actor_state->get_times_considered_and_inc();
    if (actor->simcall_.mc_max_consider_ <= actor_state->get_times_considered())
      actor_state->set_done();
  } else {
    times_considered = 0;
    actor_state->set_done();
  }

  times_considered_    = times_considered;
  aid_                 = aid;
  state->executed_req_ = actor->simcall_;

  textual_ = mc_model_checker->simcall_to_string(aid_, times_considered_);
  XBT_DEBUG("Let's run actor %ld, going for transition %s", aid, textual_.c_str());

  return replay();
}
RemotePtr<simgrid::kernel::actor::SimcallObserver> Transition::replay() const
{
  executed_transitions_++;

  simgrid::mc::RemotePtr<simgrid::kernel::actor::SimcallObserver> res = mc_model_checker->handle_simcall(*this);
  mc_model_checker->wait_for_requests();

  return res;
}

} // namespace mc
} // namespace simgrid
