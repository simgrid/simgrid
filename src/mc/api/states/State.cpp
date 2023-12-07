/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/State.hpp"
#include "src/mc/api/strategy/BasicStrategy.hpp"
#include "src/mc/api/strategy/MaxMatchComm.hpp"
#include "src/mc/api/strategy/MinMatchComm.hpp"
#include "src/mc/api/strategy/UniformStrategy.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_config.hpp"
#include "xbt/random.hpp"

#include <algorithm>
#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc, "Logging specific to MC states");

namespace simgrid::mc {

long State::expended_states_ = 0;

State::State(RemoteApp& remote_app) : num_(++expended_states_)
{
  XBT_VERB("Creating a guide for the state");

  if (_sg_mc_strategy == "none")
    strategy_ = std::make_shared<BasicStrategy>();
  else if (_sg_mc_strategy == "max_match_comm")
    strategy_ = std::make_shared<MaxMatchComm>();
  else if (_sg_mc_strategy == "min_match_comm")
    strategy_ = std::make_shared<MinMatchComm>();
  else if (_sg_mc_strategy == "uniform") {
    xbt::random::set_mersenne_seed(_sg_mc_random_seed);
    strategy_ = std::make_shared<UniformStrategy>();
  } else
    THROW_IMPOSSIBLE;

  remote_app.get_actors_status(strategy_->actors_to_run_);
}

State::State(RemoteApp& remote_app, std::shared_ptr<State> parent_state)
    : incoming_transition_(parent_state->get_transition_out()), num_(++expended_states_), parent_state_(parent_state)
{
  if (_sg_mc_strategy == "none")
    strategy_ = std::make_shared<BasicStrategy>();
  else if (_sg_mc_strategy == "max_match_comm")
    strategy_ = std::make_shared<MaxMatchComm>();
  else if (_sg_mc_strategy == "min_match_comm")
    strategy_ = std::make_shared<MinMatchComm>();
  else if (_sg_mc_strategy == "uniform")
    strategy_ = std::make_shared<UniformStrategy>();
  else
    THROW_IMPOSSIBLE;
  strategy_->copy_from(parent_state_->strategy_.get());

  remote_app.get_actors_status(strategy_->actors_to_run_);
}

std::size_t State::count_todo() const
{
  return boost::range::count_if(this->strategy_->actors_to_run_, [](auto& pair) { return pair.second.is_todo(); });
}

std::size_t State::count_todo_multiples() const
{
  size_t count = 0;
  for (auto const& [_, actor] : strategy_->actors_to_run_)
    if (actor.is_todo())
      count += actor.get_times_not_considered();

  return count;
}

aid_t State::next_transition() const
{
  XBT_DEBUG("Search for an actor to run. %zu actors to consider", strategy_->actors_to_run_.size());
  for (auto const& [aid, actor] : strategy_->actors_to_run_) {
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

std::pair<aid_t, int> State::next_transition_guided() const
{
  return strategy_->next_transition();
}

// This should be done in GuidedState, or at least interact with it
std::shared_ptr<Transition> State::execute_next(aid_t next, RemoteApp& app)
{
  // First, warn the guide, so it knows how to build a proper child state
  strategy_->execute_next(next, app);

  // This actor is ready to be executed. Execution involves three phases:

  // 1. Identify the appropriate ActorState to prepare for execution
  // when simcall_handle will be called on it
  auto& actor_state                        = strategy_->actors_to_run_.at(next);
  const unsigned times_considered          = actor_state.do_consider();
  const auto* expected_executed_transition = actor_state.get_transition(times_considered).get();
  xbt_assert(actor_state.is_enabled(), "Tried to execute a disabled actor");
  xbt_assert(expected_executed_transition != nullptr,
             "Expected a transition with %u times considered to be noted in actor %ld", times_considered, next);

  XBT_DEBUG("Let's run actor %ld (times_considered = %u)", next, times_considered);

  // 2. Execute the actor according to the preparation above
  Transition::executed_transitions_++;
  auto* just_executed = app.handle_simcall(next, times_considered, true);
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
  outgoing_transition_ = std::shared_ptr<Transition>(just_executed);

  actor_state.set_transition(outgoing_transition_, times_considered);
  app.wait_for_requests();

  return outgoing_transition_;
}

std::unordered_set<aid_t> State::get_backtrack_set() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_todo() || state.is_done()) {
      actors.insert(aid);
    }
  }
  return actors;
}

std::unordered_set<aid_t> State::get_enabled_actors() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_enabled()) {
      actors.insert(aid);
    }
  }
  return actors;
}

std::vector<aid_t> State::get_batrack_minus_done() const
{
  std::vector<aid_t> actors;
  for (const auto& [aid, state] : get_actors_list()) {
    if (state.is_todo()) {
      actors.insert(actors.begin(), aid);
    }
  }
  return actors;
}

} // namespace simgrid::mc
