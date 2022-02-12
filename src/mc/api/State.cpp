/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/State.hpp"
#include "src/mc/Session.hpp"
#include "src/mc/api.hpp"
#include "src/mc/mc_config.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

using simgrid::mc::remote;
using api = simgrid::mc::Api;

namespace simgrid {
namespace mc {

long State::expended_states_ = 0;

State::State() : num_(++expended_states_)
{
  const unsigned long maxpid = api::get().get_maxpid();
  actor_states_.resize(maxpid);
  transition_.reset(new Transition());
  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    auto snapshot_ptr = api::get().take_snapshot(num_);
    system_state_     = std::shared_ptr<simgrid::mc::Snapshot>(snapshot_ptr);
    if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
      copy_incomplete_comm_pattern();
      copy_index_comm_pattern();
    }
  }
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->actor_states_, [](simgrid::mc::ActorState const& a) { return a.is_todo(); });
}

Transition* State::get_transition() const
{
  return transition_.get();
}

int State::next_transition() const
{
  std::vector<ActorInformation>& actors = mc_model_checker->get_remote_process().actors();
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", actors.size());
  for (unsigned int i = 0; i < actors.size(); i++) {
    aid_t aid                     = actors[i].copy.get_buffer()->get_pid();
    const ActorState* actor_state = &actor_states_[aid];

    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application*/
    if (not actor_state->is_todo() || not simgrid::mc::session_singleton->actor_is_enabled(aid))
      continue;

    return i;
  }
  return -1;
}
void State::execute_next(int next)
{
  std::vector<ActorInformation>& actors = mc_model_checker->get_remote_process().actors();

  const kernel::actor::ActorImpl* actor = actors[next].copy.get_buffer();
  aid_t aid                       = actor->get_pid();
  int times_considered;

  simgrid::mc::ActorState* actor_state = &actor_states_[aid];
  /* This actor is ready to be executed. Prepare its execution when simcall_handle will be called on it */
  if (actor->simcall_.observer_ != nullptr) {
    times_considered = actor_state->get_times_considered_and_inc();
    if (actor->simcall_.mc_max_consider_ <= actor_state->get_times_considered())
      actor_state->set_done();
  } else {
    times_considered = 0;
    actor_state->set_done();
  }

  executed_req_ = actor->simcall_;

  XBT_DEBUG("Let's run actor %ld (times_considered = %d)", aid, times_considered);

  Transition::executed_transitions_++;

  transition_.reset(mc_model_checker->handle_simcall(aid, times_considered, true));
  mc_model_checker->wait_for_requests();
}

void State::copy_incomplete_comm_pattern()
{
  incomplete_comm_pattern_.clear();
  const unsigned long maxpid = api::get().get_maxpid();
  for (unsigned long i = 0; i < maxpid; i++) {
    std::vector<simgrid::mc::PatternCommunication> res;
    for (auto const& comm : incomplete_communications_pattern[i])
      res.push_back(comm->dup());
    incomplete_comm_pattern_.push_back(std::move(res));
  }
}

void State::copy_index_comm_pattern()
{
  communication_indices_.clear();
  for (auto const& list_process_comm : initial_communications_pattern)
    this->communication_indices_.push_back(list_process_comm.index_comm);
}

} // namespace mc
} // namespace simgrid
