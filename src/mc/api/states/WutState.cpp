/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/WutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_wutstate, mc_state, "States using wakeup tree for ODPOR algorithm");

namespace simgrid::mc {

WutState::WutState(RemoteApp& remote_app) : SleepSetState(remote_app)
{
  initialize_if_empty_wut();
}

WutState::WutState(RemoteApp& remote_app, StatePtr parent_state, bool initialize_wut)
    : SleepSetState(remote_app, parent_state)
{
  if (not initialize_wut) // We don't initialize the WUT when running in BeFS order
    return;

  auto parent = static_cast<WutState*>(parent_state.get());

  xbt_assert(parent != nullptr, "Attempting to construct a wakeup tree for the root state "
                                "(or what appears to be, rather for state without a parent defined)");

  XBT_DEBUG("Initializing Wut with parent one:");
  XBT_DEBUG("\n%s", parent->wakeup_tree_.string_of_whole_tree().c_str());

  const auto min_process_node = parent->wakeup_tree_.get_min_single_process_node();
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
    xbt_abort();
  }
  wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(min_process_node.value());
  initialize_if_empty_wut();
}

aid_t WutState::next_odpor_transition() const
{
  return wakeup_tree_.get_min_single_process_actor().value_or(-1);
}

void WutState::initialize_if_empty_wut()
{
  xbt_assert(!has_initialized_wakeup_tree);
  has_initialized_wakeup_tree = true;

  if (wakeup_tree_.empty())
    add_arbitrary_todo();
}

void WutState::add_arbitrary_todo()
{
  // Find an enabled transition to pick
  auto const [best_actor, _] = this->strategy_->best_transition(false);
  if (best_actor == -1)
    return; // This means that no transitions are enabled at this point
  xbt_assert(sleep_set_.find(best_actor) == sleep_set_.end(),
             "Why is a transition in a sleep set not marked as done? <%ld, %s> is in the sleep set", best_actor,
             sleep_set_.find(best_actor)->second->to_string().c_str());
  this->strategy_->consider_one(best_actor);
  auto actor_state = get_actors_list().at(best_actor);
  // For each variant of the transition that is enabled, we want to insert the action into the tree.
  // This ensures that all variants are searched
  for (unsigned times = 0; times < actor_state.get_max_considered(); ++times) {
    wakeup_tree_.insert_at_root(actor_state.get_transition(times));
  }
}

void WutState::seed_wakeup_tree_if_needed(const odpor::Execution& prior)
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
          wakeup_tree_.insert(odpor::PartialExecution{actor.get_transition(times)});
        }
        break; // Only one actor gets inserted (see pseudocode)
      }
    }
  }
  has_initialized_wakeup_tree = true;
}

void WutState::sprout_tree_from_parent_state()
{
  xbt_assert(get_parent_state() != nullptr, "Attempting to construct a wakeup tree for the root state "
                                            "(or what appears to be, rather for state without a parent defined)");

  auto parent = static_cast<WutState*>(get_parent_state());

  XBT_DEBUG("Initializing Wut with parent one:");
  XBT_DEBUG("\n%s", parent->wakeup_tree_.string_of_whole_tree().c_str());

  const auto min_process_node = parent->wakeup_tree_.get_min_single_process_node();
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

void WutState::remove_subtree_at_aid(const aid_t proc)
{
  wakeup_tree_.remove_subtree_at_aid(proc);
}

odpor::InsertionResult WutState::insert_into_wakeup_tree(const odpor::PartialExecution& pe)
{
  return this->wakeup_tree_.insert(pe);
}

void WutState::remove_subtree_using_children_in_transition(const std::shared_ptr<Transition> transition)
{
  xbt_assert(transition != nullptr, "Children state should always have an in_transition");
  if (const auto min_process_node = wakeup_tree_.get_min_single_process_node(); min_process_node.has_value()) {
    xbt_assert((transition->aid_ == min_process_node.value()->get_actor()) &&
                   (transition->type_ == min_process_node.value()->get_action()->type_),
               "We tried to make a subtree from a parent state who claimed to have executed `%s` "
               "but whose wakeup tree indicates it should have executed `%s`. This indicates "
               "that exploration is not following ODPOR. Are you sure you're choosing actors "
               "to schedule from the wakeup tree?",
               transition->to_string(false).c_str(), min_process_node.value()->get_action()->to_string(false).c_str());
  }

  wakeup_tree_.remove_min_single_process_subtree();
}

void WutState::do_odpor_unwind()
{
  XBT_DEBUG("Unwinding ODPOR from state %ld", get_num());
  xbt_assert(get_parent_state() != nullptr, "ODPOR shouldn't try to unwind from root state");

  auto parent = static_cast<WutState*>(get_parent_state());
  parent->remove_subtree_using_children_in_transition(get_transition_in());

  // Only when we've exhausted all variants of the transition which
  // can be chosen from this state do we finally add the actor to the
  // sleep set. This ensures that the current logic handling sleep sets
  // works with ODPOR in the way we intend it to work. There is not a
  // good way to perform transition equality in SimGrid; instead, we
  // effectively simply check for the presence of an actor in the sleep set.
  if (not parent->get_actors_list().at(get_transition_in()->aid_).has_more_to_consider()) {
    parent->add_sleep_set((get_transition_in()));
    parent->get_actor_at(get_transition_in()->aid_).mark_done();
  }
}

} // namespace simgrid::mc
