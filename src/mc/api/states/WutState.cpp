/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/states/WutState.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/WakeupTree.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_wutstate, mc_state, "States using wakeup tree for ODPOR algorithm");

namespace simgrid::mc {

StatePtr WutState::insert_into_tree(odpor::PartialExecution& w, RemoteApp& remote_app)
{
  XBT_DEBUG("Inserting at state #%ld sequence\n%s", get_num(), odpor::one_string_textual_trace(w).c_str());

  if (w.size() == 0)
    return nullptr;

  if (this->is_a_leaf) {
    // If the state considered is a leaf
    if (this->being_explored.test_and_set()) {
      // ... and it is already being explored by someone
      // then wait for a child to appear
      while (this->is_a_leaf) {
      } // busy-waiting
    }

    // Else, we just set the value ourself: we are responsible for this leaf exploration!
  }

  for (auto& t : this->opened_) {
    auto aid              = t->aid_;
    auto times_considered = t->times_considered_;
    StatePtr state        = this->children_states_[aid][times_considered];
    if (state == nullptr)
      continue;

    const auto& next_E_p = state->get_transition_in();

    XBT_DEBUG("... considering state after transition Actor %ld:%s as a candidate", next_E_p->aid_,
              next_E_p->to_string().c_str());

    // Is `p in `I_[E](w)`?
    if (const aid_t p = next_E_p->aid_; odpor::Execution::is_initial_after_execution_of(w, p)) {
      // Remove `p` from w and continue with this node

      // INVARIANT: If `p` occurs in `w`, it had better refer to the same
      // transition referenced by `v`. Unfortunately, we have two
      // sources of truth here which can be manipulated at the same
      // time as arguments to the function. If ODPOR works correctly,
      // they should always refer to the same value; but as a sanity check,
      // we have an assert that tests that at least the types are the same.
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w != w.end(), "Invariant violated: actor `p` "
                                              "is claimed to be an initial after `w` but is "
                                              "not actually contained in `w`. This indicates that there "
                                              "is a bug computing initials");
      const auto& w_action = *action_by_p_in_w;
      xbt_assert(w_action->type_ == next_E_p->type_,
                 "Invariant violated: `v` claims that actor `%ld` executes '%s' while "
                 "`w` claims that it executes '%s'. These two partial executions both "
                 "refer to `next_[E](p)`, which should be the same",
                 p, next_E_p->to_string(false).c_str(), w_action->to_string(false).c_str());
      w.erase(action_by_p_in_w);
      auto state_wut = static_cast<WutState*>(state.get());
      return state_wut->insert_into_tree(w, remote_app);
    }
    // Is `E ⊢ p ◇ w`?
    else if (odpor::Execution::is_independent_with_execution_of(w, next_E_p)) {
      // Nothing to remove, we simply move on

      // INVARIANT: Note that it is impossible for `p` to be
      // excluded from the set `I_[E](w)` BUT ALSO be contained in
      // `w` itself if `E ⊢ p ◇ w` (intuitively, the fact that `E ⊢ p ◇ w`
      // means that are able to move `p` anywhere in `w` IF it occurred, so
      // if it really does occur we know it must then be an initial).
      // We assert this is the case here
      const auto action_by_p_in_w =
          std::find_if(w.begin(), w.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w == w.end(),
                 "Invariant violated: We claimed that actor `%ld` is not an initial "
                 "after `w`, yet it's independent with all actions of `w` AND occurs in `w`."
                 "This indicates that there is a bug computing initials",
                 p);
      auto state_wut = static_cast<WutState*>(state.get());
      return state_wut->insert_into_tree(w, remote_app);
    }
  }

  StatePtr current_state = this;
  StatePtr parent_state  = this->get_parent_state();
  auto tran_it           = w.begin();

  parent_state = current_state;
  XBT_DEBUG("Creating state after actor %ld in parent state %ld", (*tran_it)->aid_, current_state->get_num());
  current_state = StatePtr(new WutState(remote_app, current_state, (*tran_it), false), true);

  // We need to mark the option of the first state as TODO in order to take this branch at some point
  // but this can only be done if the actor status is set
  // Also, we do this AFTER creating a child: this way the initialize will happen the transition already in opened_
  if (actor_status_set_)
    actors_to_run_[(*tran_it)->aid_]->mark_todo();

  tran_it++;

  for (; tran_it != w.end(); tran_it++) {
    parent_state = current_state;
    XBT_DEBUG("Creating state after actor %ld in parent state %ld", (*tran_it)->aid_, current_state->get_num());
    current_state = StatePtr(new WutState(remote_app, current_state, (*tran_it), false), true);
  }
  // Mark the last state (the leaf) as not being explored (ie. someone need to transform it into a node later)
  current_state->being_explored.clear();
  return current_state;
}

std::unordered_set<aid_t> WutState::get_sleeping_actors(aid_t after_actor) const
{
  std::unordered_set<aid_t> actors;
  for (const auto& [aid, _] : get_sleep_set()) {
    xbt_assert(aid != 0);
    actors.insert(aid);
  }
  // Access them directly to ensure the order of traversal
  for (size_t i = 0; i < opened_.size(); i++) {
    const auto& t = opened_[i];
    if (t->aid_ == after_actor)
      break;
    xbt_assert(t->aid_ != 0);

    actors.insert(t->aid_);
  }
  return actors;
}

} // namespace simgrid::mc
