/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/ODPOR.hpp"
#include "xbt/log.h"

#include "src/mc/api/states/State.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_odpor, mc_reduction, "Logging specific to the odpor reduction");

namespace simgrid::mc {

void ODPOR::races_computation(odpor::Execution E, stack_t* S)
{
  State* s = S->back().get();
  // ODPOR only look for race on the maximal executions
  if (not s->get_enabled_actors().empty())
    return;

  const auto last_event = E.get_latest_event_handle();

  /**
   * ODPOR Race Detection Procedure:
   *
   * For each reversible race in the current execution, we note if there are any continuations `C` equivalent to that
   * which would reverse the race that have already either a) been searched by ODPOR or b) been *noted* to be searched
   * by the wakeup tree at the appropriate reversal point, either as `C` directly or an as equivalent to `C`
   * ("eventually looks like C", viz. the `~_E` relation)
   */
  for (auto e_prime = static_cast<odpor::Execution::EventHandle>(0); e_prime <= last_event.value(); ++e_prime) {
    for (const auto e : E.get_reversible_races_of(e_prime)) {
      State& prev_state = *(*S)[e];
      if (const auto v = E.get_odpor_extension_from(e, e_prime, prev_state); v.has_value())
        prev_state.insert_into_wakeup_tree(v.value(), E.get_prefix_before(e));
    }
  }
}

bool ODPOR::has_to_be_explored(odpor::Execution E, stack_t* S)
{

  State* s = S->back().get();

  // if no actor can be executed, then return
  if (s->get_enabled_actors().empty())
    return false;

  s->seed_wakeup_tree_if_needed(E);
  return true;
}

aid_t ODPOR::next_to_explore(odpor::Execution E, stack_t* S)
{
  State* s         = S->back().get();
  const aid_t next = s->next_odpor_transition();

  if (next == -1)
    return -1;

  if (not s->is_actor_enabled(next)) {
    XBT_DEBUG("ODPOR wants to execute a disabled transition %s.",
              s->get_actors_list().at(next).get_transition()->to_string(true).c_str());
    s->remove_subtree_at_aid(next);
    s->add_sleep_set(s->get_actors_list().at(next).get_transition());
    return -1;
  }
  return next;
}
std::shared_ptr<State> ODPOR::state_create(RemoteApp& remote_app, std::shared_ptr<State> parent_state)
{
  auto s = Reduction::state_create(remote_app, parent_state);

  if (parent_state != nullptr) {
    XBT_DEBUG("Initializing Wut with parent one:");
    XBT_DEBUG("\n%s", s->parent_state_->wakeup_tree_.string_of_whole_tree().c_str());

    xbt_assert(s->parent_state_ != nullptr, "Attempting to construct a wakeup tree for the root state "
                                            "(or what appears to be, rather for state without a parent defined)");
    const auto min_process_node = s->parent_state_->wakeup_tree_.get_min_single_process_node();
    xbt_assert(min_process_node.has_value(), "Attempting to construct a subtree for a substate from a "
                                             "parent with an empty wakeup tree. This indicates either that ODPOR "
                                             "actor selection in State.cpp is incorrect, or that the code "
                                             "deciding when to make subtrees in ODPOR is incorrect");
    if (not(s->get_transition_in()->aid_ == min_process_node.value()->get_actor() &&
            s->get_transition_in()->type_ == min_process_node.value()->get_action()->type_)) {
      XBT_ERROR("We tried to make a subtree from a parent state who claimed to have executed `%s` on actor %ld "
                "but whose wakeup tree indicates it should have executed `%s` on actor %ld. This indicates "
                "that exploration is not following ODPOR. Are you sure you're choosing actors "
                "to schedule from the wakeup tree? Trace so far:",
                s->get_transition_in()->to_string(false).c_str(), s->get_transition_in()->aid_,
                min_process_node.value()->get_action()->to_string(false).c_str(),
                min_process_node.value()->get_actor());
      xbt_abort();
    }
    s->wakeup_tree_ = odpor::WakeupTree::make_subtree_rooted_at(min_process_node.value());
  }
  return s;
}

void ODPOR::on_backtrack(State* s)
{
  s->do_odpor_unwind();
}

} // namespace simgrid::mc
