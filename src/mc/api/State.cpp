/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/State.hpp"
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

  /* Copy the sleep set and eventually removes things from it: */
  /* For each actor in the previous sleep set, keep it if it is not dependent with current transition.
   * And if we kept it and the actor is enabled in this state, mark the actor as already done, so that
   * it is not explored*/
  for (const auto& [aid, transition] : parent_state_->get_sleep_set()) {
    if (not incoming_transition_->depends(transition.get())) {
      sleep_set_.try_emplace(aid, transition);
      if (strategy_->actors_to_run_.count(aid) != 0) {
        XBT_DEBUG("Actor %ld will not be explored, for it is in the sleep set", aid);
        strategy_->actors_to_run_.at(aid).mark_done();
      }
    } else
      XBT_DEBUG("Transition >>%s<< removed from the sleep set because it was dependent with incoming >>%s<<",
                transition->to_string().c_str(), incoming_transition_->to_string().c_str());
  }
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

aid_t State::next_odpor_transition() const
{
  return wakeup_tree_.get_min_single_process_actor().value_or(-1);
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

std::unordered_set<aid_t> State::get_sleeping_actors() const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    actors.insert(aid);
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

void State::seed_wakeup_tree_if_needed(const odpor::Execution& prior)
{
  // TODO: It would be better not to have such a flag.
  if (has_initialized_wakeup_tree) {
    XBT_DEBUG("Reached a node with the following initialized WuT:");
    XBT_DEBUG("\n%s", wakeup_tree_.string_of_whole_tree().c_str());

    return;
  }
  // TODO: Note that the next action taken by the actor may be updated
  // after it executes. But we will have already inserted it into the
  // tree and decided upon "happens-before" at that point for different
  // executions :(
  if (wakeup_tree_.empty()) {
    // Find an enabled transition to pick
    for (const auto& [_, actor] : get_actors_list()) {
      if (actor.is_enabled()) {
        // For each variant of the transition that is enabled, we want to insert the action into the tree.
        // This ensures that all variants are searched
        for (unsigned times = 0; times < actor.get_max_considered(); ++times) {
          wakeup_tree_.insert(prior, odpor::PartialExecution{actor.get_transition(times)});
        }
        break; // Only one actor gets inserted (see pseudocode)
      }
    }
  }
  has_initialized_wakeup_tree = true;
}

void State::sprout_tree_from_parent_state()
{

  XBT_DEBUG("Initializing Wut with parent one:");
  XBT_DEBUG("\n%s", parent_state_->wakeup_tree_.string_of_whole_tree().c_str());

  xbt_assert(parent_state_ != nullptr, "Attempting to construct a wakeup tree for the root state "
                                       "(or what appears to be, rather for state without a parent defined)");
  const auto min_process_node = parent_state_->wakeup_tree_.get_min_single_process_node();
  xbt_assert(min_process_node.has_value(), "Attempting to construct a subtree for a substate from a "
                                           "parent with an empty wakeup tree. This indicates either that ODPOR "
                                           "actor selection in State.cpp is incorrect, or that the code "
                                           "deciding when to make subtrees in ODPOR is incorrect");
  if (not(get_transition_in()->aid_ == min_process_node.value()->get_actor() &&
          get_transition_in()->type_ == min_process_node.value()->get_action()->type_)) {
    XBT_ERROR("We tried to make a subtree from a parent state who claimed to have executed `%s` on actor %ld "
              "but whose wakeup tree indicates it should have executed `%s` on actor %ld. This indicates "
              "that exploration is not following ODPOR. Are you sure you're choosing actors "
              "to schedule from the wakeup tree? Trace so far:",
              get_transition_in()->to_string(false).c_str(), get_transition_in()->aid_,
              min_process_node.value()->get_action()->to_string(false).c_str(), min_process_node.value()->get_actor());
    for (auto const& elm : Exploration::get_instance()->get_textual_trace())
      XBT_ERROR("%s", elm.c_str());
    xbt_abort();
  }
  this->wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(min_process_node.value());
}

void State::remove_subtree_using_current_out_transition()
{
  if (auto out_transition = get_transition_out(); out_transition != nullptr) {
    if (const auto min_process_node = wakeup_tree_.get_min_single_process_node(); min_process_node.has_value()) {
      xbt_assert((out_transition->aid_ == min_process_node.value()->get_actor()) &&
                     (out_transition->type_ == min_process_node.value()->get_action()->type_),
                 "We tried to make a subtree from a parent state who claimed to have executed `%s` "
                 "but whose wakeup tree indicates it should have executed `%s`. This indicates "
                 "that exploration is not following ODPOR. Are you sure you're choosing actors "
                 "to schedule from the wakeup tree?",
                 out_transition->to_string(false).c_str(),
                 min_process_node.value()->get_action()->to_string(false).c_str());
    }
  }
  wakeup_tree_.remove_min_single_process_subtree();
}

void State::remove_subtree_at_aid(const aid_t proc)
{
  wakeup_tree_.remove_subtree_at_aid(proc);
}

odpor::WakeupTree::InsertionResult State::insert_into_wakeup_tree(const odpor::PartialExecution& pe,
                                                                  const odpor::Execution& E)
{
  return this->wakeup_tree_.insert(E, pe);
}

void State::do_odpor_unwind()
{
  if (auto out_transition = get_transition_out(); out_transition != nullptr) {
    remove_subtree_using_current_out_transition();

    // Only when we've exhausted all variants of the transition which
    // can be chosen from this state do we finally add the actor to the
    // sleep set. This ensures that the current logic handling sleep sets
    // works with ODPOR in the way we intend it to work. There is not a
    // good way to perform transition equality in SimGrid; instead, we
    // effectively simply check for the presence of an actor in the sleep set.
    if (not get_actors_list().at(out_transition->aid_).has_more_to_consider())
      add_sleep_set(std::move(out_transition));
  }
}

} // namespace simgrid::mc
