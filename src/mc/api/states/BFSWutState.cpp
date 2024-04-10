/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/BFSWutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/log.h"
#include <algorithm>
#include <limits>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_bfswutstate, mc_state,
                                "Model-checker state dedicated to the BFS version of ODPOR algorithm");

namespace simgrid::mc {

BFSWutState::BFSWutState(RemoteApp& remote_app) : WutState(remote_app)
{
  aid_t chosen_actor = wakeup_tree_.get_min_single_process_actor().value();
  auto actor_state   = get_actors_list().at(chosen_actor);
  // For each variant of the transition that is enabled, we want to insert the action into the tree.
  // This ensures that all variants are searched
  for (unsigned times = 0; times < actor_state.get_max_considered(); ++times) {
    final_wakeup_tree_.insert_at_root(actor_state.get_transition(times));
  }
}

BFSWutState::BFSWutState(RemoteApp& remote_app, StatePtr parent_state) : WutState(remote_app, parent_state, true)
{
  auto parent = static_cast<BFSWutState*>(parent_state.get());
  for (const aid_t actor : parent->done_) {
    auto transition_in_done_set = parent->get_actors_list().at(actor).get_transition();
    if (not get_transition_in()->depends(transition_in_done_set.get())) {
      add_sleep_set(transition_in_done_set);
    }
  }

  aid_t incoming_actor = parent_state->get_transition_out()->aid_;
  parent->done_.push_back(incoming_actor);

  auto child_node = parent->final_wakeup_tree_.get_node_after_actor(incoming_actor);
  xbt_assert(child_node != nullptr,
             "Since this state is created from something, it should exist in its parent final_wakeup_tree! Fix Me");
  final_wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(child_node);

  wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(parent->wakeup_tree_.get_node_after_actor(incoming_actor));

  parent->wakeup_tree_.remove_subtree_at_aid(incoming_actor);

  if (not wakeup_tree_.empty())
    // We reach a state that is already guided by a wakeup tree
    // We don't need to worry about initializing it
    return;

  initialize_if_empty_wut();
  auto choice = wakeup_tree_.get_min_single_process_actor();

  if (not choice.has_value())
    // This means that we reached the end of the exploration, no more actor to consider here
    return;

  auto actor_state = get_actors_list().at(choice.value());
  // For each variant of the transition that is enabled, we want to insert the action into the tree.
  // This ensures that all variants are searched
  for (unsigned times = 0; times < actor_state.get_max_considered(); ++times) {
    final_wakeup_tree_.insert_at_root(actor_state.get_transition(times));
  }
}

void BFSWutState::record_child_state(StatePtr child)
{
  aid_t child_aid = outgoing_transition_->aid_;
  children_states_.emplace(child_aid, child);
}

std::pair<aid_t, int> BFSWutState::next_transition_guided() const
{

  aid_t best_actor = this->next_transition();
  if (best_actor == -1)
    return std::make_pair(best_actor, std::numeric_limits<int>::max());
  return std::make_pair(best_actor, strategy_->get_actor_valuation(best_actor));
}

void BFSWutState::unwind_wakeup_tree_from_parent()
{
  auto parent = static_cast<BFSWutState*>(get_parent_state());

  // For each leaf of the parent WuT corresponding to this actor,
  // try to insert the corresponding sequence. If it still correponds to a new
  // possibility, insert it for real in the WuT.
  for (auto node : *parent->wakeup_tree_.get_node_after_actor(parent->get_transition_out()->aid_)) {
    if (not node->is_leaf())
      continue;

    const odpor::PartialExecution v_prime = final_wakeup_tree_.insert_and_get_inserted_seq(node->get_sequence());
    if (not v_prime.empty())
      wakeup_tree_.insert(v_prime);
  }

  parent->wakeup_tree_.remove_subtree_at_aid(parent->get_transition_out()->aid_);
}

aid_t BFSWutState::next_transition() const
{
  int best_valuation = std::numeric_limits<int>::max();
  int best_actor     = -1;
  for (const aid_t aid : wakeup_tree_.get_direct_children_actors()) {
    if (strategy_->get_actor_valuation(aid) < best_valuation) {
      best_valuation = strategy_->get_actor_valuation(aid);
      best_actor     = aid;
    }
  }
  if (best_actor == -1)
    XBT_DEBUG("Oops, no more people here");
  return best_actor;
}

std::shared_ptr<Transition> BFSWutState::execute_next(aid_t next, RemoteApp& app)
{
  if (children_states_.find(next) != children_states_.end()) {
    outgoing_transition_ = get_actors_list().at(next).get_transition();
    XBT_DEBUG("Found an existing transition for actor %ld", next);
    return outgoing_transition_;
  }
  return State::execute_next(next, app);
}

std::unordered_set<aid_t> BFSWutState::get_sleeping_actors(aid_t after_actor) const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    actors.insert(aid);
  }
  for (const auto& aid : done_) {
    if (aid == after_actor)
      break;
    actors.insert(aid);
  }
  return actors;
}

odpor::PartialExecution BFSWutState::insert_into_final_wakeup_tree(const odpor::PartialExecution& pe)
{
  return this->final_wakeup_tree_.insert_and_get_inserted_seq(pe);
}

} // namespace simgrid::mc
