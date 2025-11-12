/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/ODPOR.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_odpor, mc_reduction, "Logging specific to the odpor reduction");

namespace simgrid::mc {

Reduction::RaceUpdate* ODPOR::races_computation(odpor::Execution& E, stack_t* S, std::vector<StatePtr>* opened_states)
{
  State* s = S->back().get();
  // ODPOR only look for race on the maximal executions
  if (not s->get_enabled_actors().empty())
    return new RaceUpdate();

  const auto last_event = E.get_latest_event_handle();
  auto updates          = new RaceUpdate();
  /**
   * ODPOR Race Detection Procedure:
   *
   * For each reversible race in the current execution, we note if there are any continuations `C` equivalent to that
   * which would reverse the race that have already either a) been searched by ODPOR or b) been *noted* to be searched
   * by the wakeup tree at the appropriate reversal point, either as `C` directly or an as equivalent to `C`
   * ("eventually looks like C", viz. the `~_E` relation)
   */
  XBT_DEBUG("Going to compute all the reversible races (size stack: %lu) on sequence (size sequence: %lu) \n%s",
            S->size(), E.size(), E.get_one_string_textual_trace().c_str());
  for (auto e_prime = static_cast<odpor::Execution::EventHandle>(0); e_prime <= last_event.value(); ++e_prime) {
    if (E.get_event_with_handle(e_prime).has_race_been_computed())
      continue;

    XBT_VERB("Computing reversible races of Event `%u`", e_prime);
    for (const auto e : E.get_reversible_races_of(e_prime, S)) {
      xbt_assert((*S)[e] != nullptr and (*S)[e_prime] != nullptr);

      XBT_DEBUG("... racing event `%u``", e);
      XBT_DEBUG("... race between event `%u`:%s and event `%u`:%s", e_prime,
                E.get_transition_for_handle(e_prime)->to_string().c_str(), e,
                E.get_transition_for_handle(e)->to_string().c_str());

      WutState* prev_state = static_cast<WutState*>((*S)[e].get());

      auto actor_after_e = ((*S)[e + 1].get())->get_transition_in()->aid_;

      if (const auto v = E.get_odpor_extension_from(e, e_prime, *prev_state, actor_after_e); v.has_value()) {
        updates->add_element(prev_state, v.value());
        XBT_DEBUG("... ... work will be added at state #%ld", prev_state->get_num());
      }
    }
    E.get_event_with_handle(e_prime).consider_races();
  }
  updates->last_explored_state_ = S->back().get();
  XBT_DEBUG("Packing a total of %lu race updates", updates->get_value().size());
  return updates;
}

unsigned long ODPOR::apply_race_update(RemoteApp& remote_app, Reduction::RaceUpdate* updates,
                                       std::vector<StatePtr>* opened_states)
{

  unsigned long nb_updates = 0;
  auto odpor_updates       = static_cast<RaceUpdate*>(updates);
  XBT_DEBUG("Applying the %lu received race updates", odpor_updates->get_value().size());
  for (auto& [state, seq] : odpor_updates->get_value()) {
    XBT_DEBUG("Going to insert sequence\n%s", odpor::one_string_textual_trace(seq).c_str());
    XBT_DEBUG("... at state #%ld", state->get_num());
    const auto inserted_state = static_cast<WutState*>(state.get())->insert_into_tree(seq, remote_app);
    if (inserted_state != nullptr) {
      XBT_DEBUG("... ended up adding work to do at state #%ld", inserted_state->get_num());

      if (opened_states != nullptr)
        opened_states->push_back(inserted_state);
      nb_updates++;
    }
  }
  return nb_updates;
}

aid_t ODPOR::next_to_explore(odpor::Execution& E, stack_t* S)
{
  auto s           = S->back().get();
  const aid_t next = Exploration::get_strategy()->next_transition_in(s).first;

  if (next == -1)
    return -1;

  // xbt_assert(s->is_actor_enabled(next), "ODPOR wants to execute a disabled transition. Fix Me!");

  return next;
}
StatePtr ODPOR::state_create(RemoteApp& remote_app, StatePtr parent_state,
                             std::shared_ptr<Transition> incoming_transition)
{
  StatePtr new_state;
  if (parent_state == nullptr)
    new_state = StatePtr(new WutState(remote_app), true);
  else {
    if (auto existing_state =
            parent_state->get_children_state_of_aid(incoming_transition->aid_, incoming_transition->times_considered_);
        existing_state != nullptr) {
      if (not existing_state->has_been_initialized()) {
        existing_state->update_incoming_transition_explicitly(incoming_transition);
        existing_state->initialize(remote_app);
        if (existing_state->next_transition() == -1)
          static_cast<SleepSetState*>(existing_state.get())->add_arbitrary_transition(remote_app);
      }
      return existing_state;
    }
    new_state = StatePtr(new WutState(remote_app, parent_state, incoming_transition), true);
  }
  static_cast<SleepSetState*>(new_state.get())->add_arbitrary_transition(remote_app);
  return new_state;
}

void ODPOR::on_backtrack(State* s)
{
  // static_cast<SleepSetState*>(s->get_parent_state())->add_sleep_set(s->get_transition_in());
}

void ODPOR::consider_best(StatePtr state)
{
  Exploration::get_strategy()->consider_best_in(state.get());
}

} // namespace simgrid::mc
