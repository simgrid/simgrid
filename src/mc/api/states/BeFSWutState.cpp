/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/BeFSWutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_befswutstate, mc_state,
                                "Model-checker state dedicated to the BeFS version of ODPOR algorithm");

namespace simgrid::mc {

void BeFSWutState::compare_final_and_wut()
{
  if (_sg_mc_debug)
    xbt_assert(wakeup_tree_.is_contained_in(final_wakeup_tree_), "final_wut: %s\n versus curren wut: %s",
               get_string_of_final_wut().c_str(), string_of_wut().c_str());
}

BeFSWutState::BeFSWutState(RemoteApp& remote_app) : WutState(remote_app)
{
  is_leftmost_ = true; // The first state is the only one at that depth, so the leftmost one.

  aid_t chosen_actor = wakeup_tree_.get_min_single_process_actor().value();
  auto actor_state   = get_actors_list().at(chosen_actor);
  // For each variant of the transition that is enabled, we want to insert the action into the tree.
  // This ensures that all variants are searched
  for (unsigned times = 0; times < actor_state.get_max_considered(); ++times) {
    final_wakeup_tree_.insert_at_root(actor_state.get_transition(times));
  }
}

BeFSWutState::BeFSWutState(RemoteApp& remote_app, StatePtr parent_state) : WutState(remote_app, parent_state, false)
{
  auto parent = static_cast<BeFSWutState*>(parent_state.get());
  for (const aid_t actor : parent->done_) {
    auto transition_in_done_set = parent->get_actors_list().at(actor).get_transition();
    if (not get_transition_in()->depends(transition_in_done_set.get()))
      sleep_add_and_mark(transition_in_done_set);
  }

  is_leftmost_ = parent->is_leftmost_ and parent->done_.size() == parent->closed_.size();

  aid_t incoming_actor = parent_state->get_transition_out()->aid_;
  parent->done_.push_back(incoming_actor);

  auto child_node = parent->final_wakeup_tree_.get_node_after_actor(incoming_actor);
  if (child_node == nullptr) {
    XBT_CRITICAL("Since this state is created from something, it should exist in its parent final_wakeup_tree! Fix Me. "
                 "I was created from transition %s. Going to display WuT from everyone of this dynastie",
                 parent->get_transition_out().get()->to_string().c_str());

    auto curr_state = this;
    while (curr_state != nullptr) {
      XBT_CRITICAL("Final WuT at state %ld:", curr_state->get_num());
      XBT_CRITICAL("%s", curr_state->get_string_of_final_wut().c_str());
      XBT_CRITICAL("Current WuT at state %ld:", curr_state->get_num());
      XBT_CRITICAL("%s", curr_state->string_of_wut().c_str());
      curr_state = static_cast<BeFSWutState*>(curr_state->get_parent_state());
    }

    xbt_die("Find the culprit and fix me!");
  }
  final_wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(child_node);

  wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(parent->wakeup_tree_.get_node_after_actor(incoming_actor));

  parent->wakeup_tree_.remove_subtree_at_aid(incoming_actor);
  compare_final_and_wut();
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
  compare_final_and_wut();
}

void BeFSWutState::record_child_state(StatePtr child)
{
  aid_t child_aid = outgoing_transition_->aid_;
  children_states_.emplace(child_aid, child);
}

std::pair<aid_t, int> BeFSWutState::next_transition_guided() const
{

  aid_t best_actor = this->next_transition();
  if (best_actor == -1)
    return std::make_pair(best_actor, std::numeric_limits<int>::max());
  return std::make_pair(best_actor, strategy_->get_actor_valuation(best_actor));
}

void BeFSWutState::unwind_wakeup_tree_from_parent()
{
  auto parent = static_cast<BeFSWutState*>(get_parent_state());

  // For each leaf of the parent WuT corresponding to this actor,
  // try to insert the corresponding sequence. If it still correponds to a new
  // possibility, insert it for real in the WuT.
  for (odpor::WakeupTreeNode* node : *parent->wakeup_tree_.get_node_after_actor(parent->get_transition_out()->aid_)) {
    if (not node->is_leaf())
      continue;

    XBT_DEBUG("Going to insert sequence from parent to children %s",
              node->string_of_whole_tree("", false, true).c_str());

    // Remove the aid from the sequence
    auto v = node->get_sequence();
    v.erase(v.begin());

    const odpor::PartialExecution v_prime = final_wakeup_tree_.insert_and_get_inserted_seq(v);
    if (not v_prime.empty())
      wakeup_tree_.insert(v_prime);
  }

  parent->wakeup_tree_.remove_subtree_at_aid(parent->get_transition_out()->aid_);
  compare_final_and_wut();
}

aid_t BeFSWutState::next_transition() const
{
  int best_valuation = std::numeric_limits<int>::max();
  int best_actor     = -1;
  for (const aid_t aid : wakeup_tree_.get_direct_children_actors()) {
    if (get_actors_list().find(aid) ==
        get_actors_list().end()) { // the tree is asking to execute someone that doesn't exist in the actor list!
      XBT_CRITICAL("Find a tree pretending that it could execute %ld while this actor is not a possibility (%lu "
                   "existing actors from this state). Find the culprit, and Fix Me. Error @state %ld",
                   aid, get_actors_list().size(), this->get_num());
      XBT_CRITICAL("The existing actors are:");
      for (auto& [aid, astate] : get_actors_list()) {
        XBT_CRITICAL("Actor %ld: %s", aid, astate.get_transition()->to_string().c_str());
      }
      XBT_CRITICAL("While the WuT is:\n%s\n", wakeup_tree_.string_of_whole_tree().c_str());
      xbt_die("Fix me!");
    }
    if (strategy_->get_actor_valuation(aid) < best_valuation) {
      best_valuation = strategy_->get_actor_valuation(aid);
      best_actor     = aid;
    }
  }
  if (best_actor == -1)
    XBT_DEBUG("Oops, no more people here");
  return best_actor;
}

std::unordered_set<aid_t> BeFSWutState::get_sleeping_actors(aid_t after_actor) const
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

odpor::PartialExecution BeFSWutState::insert_into_final_wakeup_tree(const odpor::PartialExecution& pe)
{
  return this->final_wakeup_tree_.insert_and_get_inserted_seq(pe);
}

StatePtr BeFSWutState::force_insert_into_wakeup_tree(const odpor::PartialExecution& pe)
{
  if (pe.size() == 0)
    return nullptr;

  // If the start of the sequence corresponds to an already explored state
  final_wakeup_tree_.force_insert(pe);

  auto first_actor = pe.front()->aid_;
  if (StatePtr children = get_children_state_of_aid(first_actor); children != nullptr) {
    odpor::PartialExecution suffix = pe;
    suffix.erase(suffix.begin());
    return static_cast<BeFSWutState*>(children.get())->force_insert_into_wakeup_tree(std::move(suffix));
  }

  this->wakeup_tree_.force_insert(pe);
  compare_final_and_wut();
  return this;
}

BeFSWutState::~BeFSWutState()
{

  auto parent_state = get_parent_state();
  if (parent_state != nullptr) {
    auto parent = static_cast<BeFSWutState*>(parent_state);
    parent->closed_.emplace_back(get_transition_in()->aid_);
    parent->signal_on_backtrack();
  }
}

void BeFSWutState::signal_on_backtrack()
{
  XBT_DEBUG("State n°%ld, son of %ld, being signaled to backtrack", get_num(),
            get_parent_state() != nullptr ? get_parent_state()->get_num() : -1);
  XBT_DEBUG("... %s",
            is_leftmost_ ? "This state is the leftmost at this depth" : "This state is NOT the leftmost at this depth");
  XBT_DEBUG("... There are %lu done children, %lu closed children and %lu recorded children StatePtr", done_.size(),
            closed_.size(), children_states_.size());
  if (not is_leftmost_)
    return;

  if (closed_.size() < done_.size()) {

    // if there are children states that are being visited, we may need to update the leftmost information
    aid_t leftmost_aid = done_[closed_.size()];
    auto children_aid  = children_states_.find(leftmost_aid);
    xbt_assert(children_aid != children_states_.end());
    auto children_befs_aid          = static_cast<BeFSWutState*>(children_aid->second.get());
    children_befs_aid->is_leftmost_ = true;
    children_befs_aid->signal_on_backtrack();
    return;
  }

  XBT_DEBUG("... there's still at least one actor to execute (%ld)", next_transition());
  if (next_transition() == -1) {
    // This is the leftmost state, it doesn't have anymore open children and nothing left to do
    // Let's close this by removing it from it's parent
    auto parent_state = get_parent_state();
    if (parent_state != nullptr) {
      auto parent = static_cast<BeFSWutState*>(parent_state);
      XBT_DEBUG("\t... there are %lu recorded children StatePtr in its parent state n°%ld",
                parent->children_states_.size(), parent->get_num());
      auto find_aid = parent->children_states_.find(get_transition_in()->aid_);
      xbt_assert(find_aid != parent->children_states_.end());
      parent->children_states_.erase(find_aid);
    }
  }
}

} // namespace simgrid::mc
