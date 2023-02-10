/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/State.hpp"
#include "src/mc/mc_config.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_ = 0;

State::State(const RemoteApp& remote_app) : num_(++expended_states_)
{
  remote_app.get_actors_status(actors_to_run_);

  /* Stateful model checking */
  if ((_sg_mc_checkpoint > 0 && (num_ % _sg_mc_checkpoint == 0)) || _sg_mc_termination) {
    system_state_ = std::make_shared<simgrid::mc::Snapshot>(num_);
  }
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->actors_to_run_, [](auto& pair) { return pair.second.is_todo(); });
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
    if (not actor.is_todo() || not actor.is_enabled())
      continue;

    return aid;
  }
  return -1;
}
void State::execute_next(aid_t next)
{
  /* This actor is ready to be executed. Prepare its execution when simcall_handle will be called on it */
  const unsigned times_considered    = actors_to_run_.at(next).do_consider();
  auto* expected_executed_transition = actors_to_run_.at(next).get_transition(times_considered);
  xbt_assert(expected_executed_transition != nullptr,
             "Expected a transition with %d times considered to be noted in actor %lu", times_considered, next);

  XBT_DEBUG("Let's run actor %ld (times_considered = %u)", next, times_considered);

  Transition::executed_transitions_++;

  const auto* executed_transition = mc_model_checker->handle_simcall(next, times_considered, true);
  xbt_assert(executed_transition->type_ == expected_executed_transition->type_,
             "The transition that was just executed by actor %lu, viz:\n"
             "%s\n"
             "is not what was purportedly scheduled to execute, which was:\n"
             "%s\n",
             next, executed_transition->to_string().c_str(), expected_executed_transition->to_string().c_str());
  transition_ = expected_executed_transition;
  mc_model_checker->wait_for_requests();
}
} // namespace simgrid::mc
