/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_ = 0;

State::State(RemoteApp& remote_app) : num_(++expended_states_)
{
  remote_app.get_actors_status(actors_to_run_);

  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination)
    system_state_ = std::make_shared<simgrid::mc::Snapshot>(num_, remote_app.get_page_store());
}

State::State(RemoteApp& remote_app, const State* previous_state)
    : default_transition_(std::make_unique<Transition>()), num_(++expended_states_)
{

  remote_app.get_actors_status(actors_to_run_);

  transition_ = default_transition_.get();

  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    system_state_ = std::make_shared<simgrid::mc::Snapshot>(num_, remote_app.get_page_store());
  }

  /* For each actor in the previous sleep set, keep it if it is not dependent with current transition.
   * And if we kept it and the actor is enabled in this state, mark the actor as already done, so that
   * it is not explored*/
  for (auto& [aid, transition] : previous_state->get_sleep_set()) {

    if (not previous_state->get_transition()->depends(&transition)) {

      sleep_set_.emplace(aid, transition);
      if (actors_to_run_.count(aid) != 0) {
        XBT_DEBUG("Actor %ld will not be explored, for it is in the sleep set", aid);

        actors_to_run_.at(aid).mark_done();
      }
    } else
      XBT_DEBUG("Transition >>%s<< removed from the sleep set because it was dependent with >>%s<<",
                transition.to_string().c_str(), previous_state->get_transition()->to_string().c_str());
  }
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->actors_to_run_, [](auto& pair) { return pair.second.is_todo(); });
}

void State::mark_all_enabled_todo()
{
  for (auto const& [aid, _] : this->get_actors_list()) {
      if (this->is_actor_enabled(aid) and not is_actor_done(aid)) {
      this->mark_todo(aid);
    }
  }
}

Transition* State::get_transition() const
{
  return transition_;
}

aid_t State::next_transition() const
{
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", actors_to_run_.size());
  for (auto const& [aid, actor] : actors_to_run_) {
    /* Only consider actors (1) marked as interleaving by the checker and (2) currently enabled in the application */
    if (not actor.is_todo() || not actor.is_enabled() || actor.is_done()) {

      if (not actor.is_todo())
        XBT_DEBUG("Can't run actor %ld because it is not todo", aid);

      if (not actor.is_enabled())
        XBT_DEBUG("Can't run actor %ld because it is not enabled", aid);

      if (actor.is_done())
        XBT_DEBUG("Can't run actor %ld because it has already been done", aid);

      continue;
    }

    return aid;
  }
  return -1;
}

void State::execute_next(aid_t next)
{
  // This actor is ready to be executed. Execution involves three phases:

  // 1. Identify the appropriate ActorState to prepare for execution
  // when simcall_handle will be called on it
  auto& actor_state                        = actors_to_run_.at(next);
  const unsigned times_considered          = actor_state.do_consider();
  const auto* expected_executed_transition = actor_state.get_transition(times_considered);
  xbt_assert(expected_executed_transition != nullptr,
             "Expected a transition with %u times considered to be noted in actor %ld", times_considered, next);

  XBT_DEBUG("Let's run actor %ld (times_considered = %u)", next, times_considered);

  // 2. Execute the actor according to the preparation above
  Transition::executed_transitions_++;
  auto* just_executed = mc_model_checker->handle_simcall(next, times_considered, true);
  xbt_assert(just_executed->type_ == expected_executed_transition->type_,
             "The transition that was just executed by actor %ld, viz:\n"
             "%s\n"
             "is not what was purportedly scheduled to execute, which was:\n"
             "%s\n",
             next, just_executed->to_string().c_str(), expected_executed_transition->to_string().c_str());

  // 3. Update the state with the newest information. This means recording
  // both
  //  1. what action was last taken from this state (viz. `executed_transition`)
  //  2. what action actor `next` was able to take given `times_considered`
  // The latter update is important as *more* information is potentially available
  // about a transition AFTER it has executed.
  transition_ = just_executed;

  auto executed_transition = std::unique_ptr<Transition>(just_executed);
  actor_state.set_transition(std::move(executed_transition), times_considered);

  mc_model_checker->get_exploration()->get_remote_app().wait_for_requests();
}
} // namespace simgrid::mc
