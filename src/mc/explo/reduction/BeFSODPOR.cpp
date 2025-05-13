/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/BeFSODPOR.hpp"
#include "src/mc/api/states/BeFSWutState.hpp"
#include "src/mc/api/states/WutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"

#include "src/mc/mc_forward.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_befsodpor, mc_reduction, "Logging specific to the BeFS ODPOR reduction");

namespace simgrid::mc {

std::unique_ptr<Reduction::RaceUpdate> BeFSODPOR::races_computation(odpor::Execution& E, stack_t* S,
                                                                    std::vector<StatePtr>* opened_states)
{
  if (opened_states == nullptr)
    XBT_VERB("calling BeFSODPOR outside of BeFS algorithm: the only case this should happen is if you are looking for "
             "the critical transition");

  State* s = S->back().get();
  // ODPOR only look for race on the maximal executions
  if (not s->get_enabled_actors().empty()) {
    return std::make_unique<RaceUpdate>();
  }

  const auto last_event = E.get_latest_event_handle();
  auto updates          = std::make_unique<RaceUpdate>();
  /**
   * ODPOR Race Detection Procedure:
   *
   * For each reversible race in the current execution, we note if there are any continuations `C` equivalent to that
   * which would reverse the race that have already either a) been searched by ODPOR or b) been *noted* to be searched
   * by the wakeup tree at the appropriate reversal point, either as `C` directly or an as equivalent to `C`
   * ("eventually looks like C", viz. the `~_E` relation)
   */
  XBT_DEBUG("Going to compute all the reversible races on sequence \n%s", E.get_one_string_textual_trace().c_str());
  for (auto e_prime = static_cast<odpor::Execution::EventHandle>(0); e_prime <= last_event.value(); ++e_prime) {
    if (E.get_event_with_handle(e_prime).has_race_been_computed())
      continue;
    XBT_VERB("Computing reversible races of Event `%u`", e_prime);
    for (const auto e : E.get_reversible_races_of(e_prime, S)) {
      XBT_DEBUG("... racing event `%u`", e);
      XBT_DEBUG("... race between event `%u`:%s and event `%u`:%s", e_prime,
                E.get_transition_for_handle(e_prime)->to_string().c_str(), e,
                E.get_transition_for_handle(e)->to_string().c_str());
      BeFSWutState* prev_state = static_cast<BeFSWutState*>((*S)[e].get());
      xbt_assert(prev_state != nullptr, "ODPOR should use WutState. Fix me");

      auto actor_after_e = ((*S)[e + 1].get())->get_transition_in()->aid_;

      if (const auto v = E.get_odpor_extension_from(e, e_prime, *prev_state, actor_after_e); v.has_value()) {
        XBT_DEBUG("... inserting sequence %s in final_wut before event `%u`",
                  odpor::one_string_textual_trace(v.value()).c_str(), e);
        updates->add_element(prev_state, v.value());
      }
    }
    E.get_event_with_handle(e_prime).consider_races();
  }
  return updates;
}

unsigned long BeFSODPOR::apply_race_update(std::unique_ptr<Reduction::RaceUpdate> updates,
                                           std::vector<StatePtr>* opened_states)
{
  if (opened_states == nullptr)
    XBT_VERB("calling BeFS ODPOR outside of BeFS algorithm: the only case this should happen is if you are looking for "
             "the critical transition");

  auto befsodpor_updates   = static_cast<RaceUpdate*>(updates.get());
  unsigned long nb_updates = 0;
  for (auto& [raw_state, v] : befsodpor_updates->get_value()) {
    XBT_DEBUG("Going to insert sequence\n%s", odpor::one_string_textual_trace(v).c_str());
    XBT_DEBUG("... at state #%ld", raw_state->get_num());
    auto state         = static_cast<BeFSWutState*>(raw_state.get());
    const auto inserted_state = state->insert_into_tree(v);
    if (opened_states != nullptr and inserted_state != nullptr) {
      opened_states->push_back(inserted_state);
      XBT_DEBUG("... ended up adding work to do at state #%ld", inserted_state->get_num());
    }
    nb_updates++;
  }
  return nb_updates;
}

aid_t BeFSODPOR::next_to_explore(odpor::Execution& E, stack_t* S)
{
  auto s = static_cast<BeFSWutState*>(S->back().get());
  xbt_assert(s != nullptr, "BeFS ODPOR should use BeFSWutState. Fix me");
  const aid_t next = s->next_transition();

  if (next == -1)
    return -1;

  xbt_assert(s->is_actor_enabled(next), "BeFS ODPOR wants to execute a disabled transition. Fix Me!");

  return next;
}

StatePtr BeFSODPOR::state_create(RemoteApp& remote_app, StatePtr parent_state,
                                 std::shared_ptr<Transition> incoming_transition)
{
  StatePtr new_state;
  if (parent_state == nullptr)
    new_state = StatePtr(new WutState(remote_app), true);
  else {
    BeFSWutState* befswut_state = static_cast<BeFSWutState*>(parent_state.get());
    if (auto existing_state = befswut_state->get_children_state_of_aid(
            parent_state->get_transition_out()->aid_, parent_state->get_transition_out()->times_considered_);
        existing_state != nullptr) {
      xbt_die("For sure we never want to revisit something right?");
      return existing_state;
    }
    new_state = StatePtr(new BeFSWutState(remote_app, befswut_state, incoming_transition), true);
  }
  static_cast<SleepSetState*>(new_state.get())->add_arbitrary_transition(remote_app);
  return new_state;
}

} // namespace simgrid::mc
