/* Copyright (c) 2008-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/BFSODPOR.hpp"
#include "src/mc/api/states/BFSWutState.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/odpor/Execution.hpp"

#include "xbt/asserts.h"
#include "xbt/log.h"

#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_bfsodpor, mc_reduction, "Logging specific to the odpor reduction");

namespace simgrid::mc {

void BFSODPOR::races_computation(odpor::Execution& E, stack_t* S, std::vector<std::shared_ptr<State>>* opened_states)
{
  xbt_assert(opened_states != nullptr, "BFDODPOR reduction should only be used with BFS algorithm");

  State* s = S->back().get();
  // ODPOR only look for race on the maximal executions
  if (not s->get_enabled_actors().empty()) {
    auto s_cast = static_cast<BFSWutState*>(S->back().get());
    xbt_assert(s != nullptr, "BFSODPOR should use BFSWutState. Fix me");
    XBT_DEBUG("Non terminal execution reached, current WuT is the following: %s", s_cast->string_of_wut().c_str());
    if (s_cast->direct_children() > 1 and opened_states != nullptr)
      opened_states->emplace_back(S->back());
    return;
  }

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
    if (E.get_event_with_handle(e_prime).has_race_been_computed())
      continue;
    XBT_VERB("Computing reversible races of Event `%u`", e_prime);
    for (const auto e : E.get_reversible_races_of(e_prime)) {
      XBT_DEBUG("... racing event `%u``", e);
      BFSWutState* prev_state = static_cast<BFSWutState*>((*S)[e].get());
      xbt_assert(prev_state != nullptr, "ODPOR should use WutState. Fix me");

      if (const auto v = E.get_odpor_extension_from(e, e_prime, *prev_state, prev_state->get_transition_out()->aid_);
          v.has_value()) {
        XBT_DEBUG("... inserting sequence %s in final_wut before event `%u`",
                  odpor::one_string_textual_trace(v.value()).c_str(), e);
        const auto v_prime = prev_state->insert_into_final_wakeup_tree(v.value());
        XBT_DEBUG("... after insertion final_wut looks like this: %s", prev_state->get_string_of_final_wut().c_str());
        if (not v_prime.empty()) {
          XBT_DEBUG("... inserting sequence %s before event `%u`", odpor::one_string_textual_trace(v_prime).c_str(), e);
          prev_state->insert_into_wakeup_tree(v_prime);
          opened_states->push_back((*S)[e]);
        }
      }
    }
    E.get_event_with_handle(e_prime).consider_races();
  }
}

aid_t BFSODPOR::next_to_explore(odpor::Execution& E, stack_t* S)
{
  auto s = static_cast<BFSWutState*>(S->back().get());
  xbt_assert(s != nullptr, "BFSODPOR should use BFSWutState. Fix me");
  const aid_t next = s->next_transition();

  if (next == -1)
    return -1;

  if (not s->is_actor_enabled(next)) {
    XBT_DEBUG("BFSODPOR wants to execute a disabled transition %s.",
              s->get_actors_list().at(next).get_transition()->to_string(true).c_str());
    s->remove_subtree_at_aid(next);
    s->add_sleep_set(s->get_actors_list().at(next).get_transition());
    return -1;
  }

  return next;
}

std::shared_ptr<State> BFSODPOR::state_create(RemoteApp& remote_app, std::shared_ptr<State> parent_state)
{
  if (parent_state == nullptr)
    return std::make_shared<BFSWutState>(remote_app);
  else {
    std::shared_ptr<BFSWutState> bfswut_state = std::static_pointer_cast<BFSWutState>(parent_state);
    xbt_assert(bfswut_state != nullptr, "Wrong kind of state for this reduction. This shouldn't happen, fix me");
    if (auto existing_state = bfswut_state->get_children_state_of_aid(parent_state->get_transition_out()->aid_);
        existing_state != nullptr) {
      existing_state->unwind_wakeup_tree_from_parent();
      return existing_state;
    }
    auto new_state = std::make_shared<BFSWutState>(remote_app, bfswut_state);
    bfswut_state->record_child_state(new_state);
    return std::move(new_state);
  }
}

} // namespace simgrid::mc
