/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_sleepset, mc_state, "DFS exploration algorithm of the model-checker");

namespace simgrid::mc {

SleepSetState::SleepSetState(RemoteApp& remote_app) : State(remote_app) {}

SleepSetState::SleepSetState(RemoteApp& remote_app, StatePtr parent_state,
                             std::shared_ptr<Transition> incoming_transition, bool set_actor_status)
    : State(remote_app, parent_state, incoming_transition, set_actor_status)
{
  /* Copy the sleep set and eventually removes things from it: */
  /* For each actor in the previous sleep set, keep it if it is not dependent with current transition.
   * And if we kept it and the actor is enabled in this state, mark the actor as already done, so that
   * it is not explored*/
  for (size_t i = 0; i < static_cast<SleepSetState*>(parent_state.get())->opened_.size(); i++) {
    auto const& transition = static_cast<SleepSetState*>(parent_state.get())->opened_[i];
    XBT_DEBUG("At state #%ld, transition <Actor %ld: %s> is contained in parent opened", get_num(), transition->aid_,
              transition->to_string().c_str());
    if (not get_transition_in()->depends(transition.get())) {
      XBT_DEBUG("sleep set @ state #%ld: transition <Actor %ld: %s> added from parent opened set", get_num(),
                transition->aid_, transition->to_string().c_str());
      sleep_add_and_mark(transition);
    }
    if (transition->aid_ == incoming_transition->aid_)
      break;
  }

  for (const auto& [aid, transition] : static_cast<SleepSetState*>(parent_state.get())->get_sleep_set()) {
    if (not get_transition_in()->depends(transition.get())) {
      XBT_DEBUG("sleep set @ state #%ld: transition <Actor %ld: %s> added from parent sleep set", get_num(),
                transition->aid_, transition->to_string().c_str());
      sleep_add_and_mark(transition);
    }
  }

  if (not sleep_set_.empty() and parent_state->has_correct_execution())
    this->register_as_correct(); // FIX ME
  // This is only working if the parent has been fully explored when creating this state
  // In other word, if we are doing any sort of BeFS, there are no good reason for this to work as intented
}

void SleepSetState::add_arbitrary_transition(RemoteApp& remote_app)
{
  XBT_DEBUG("Adding arbitraty transition in state #%ld", get_num());
  if (sleep_set_.empty() and Exploration::can_go_one_way()) {
    XBT_DEBUG("Asking for one way");
    xbt_assert(next_transition() == -1,
               "State #%ld already has something to explore, why are we adding an arbitrary transition there?",
               get_num());
    remote_app.go_one_way();
    aid_t aid = remote_app.get_aid_of_next_transition();
    if (aid == -1) {
      xbt_assert(Exploration::get_strategy()->best_transition_in(this, false).first == -1);
      return; // No more enabled actors here
    }
    consider_one(aid);
  } else {
    Exploration::get_strategy()->consider_best_in(this);
  }
}

void SleepSetState::sleep_add_and_mark(std::shared_ptr<Transition> transition)
{
  XBT_DEBUG("Adding transition Actor %ld:%s to the sleep set from parent state", transition->aid_,
            transition->to_string().c_str());
  sleep_set_.try_emplace(transition->aid_, transition);
  if (actors_to_run_.size() > (unsigned)transition->aid_ and actors_to_run_[transition->aid_].has_value()) {
    actors_to_run_[transition->aid_]->mark_done();
  }
}

std::unordered_set<aid_t> SleepSetState::get_sleeping_actors(aid_t) const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    actors.insert(aid);
  }
  return actors;
}
std::vector<aid_t> SleepSetState::get_enabled_minus_sleep() const
{
  std::vector<aid_t> actors;
  for (const auto& state : actors_to_run_) {
    if (not state.has_value())
      continue;
    if (state->is_enabled() && sleep_set_.count(state->get_aid()) < 1) {
      actors.insert(actors.begin(), state->get_aid());
    }
  }
  return actors;
}

bool SleepSetState::is_actor_sleeping(aid_t actor) const
{
  return std::find_if(sleep_set_.begin(), sleep_set_.end(), [=](const auto& pair) { return pair.first == actor; }) !=
         sleep_set_.end();
}

} // namespace simgrid::mc
