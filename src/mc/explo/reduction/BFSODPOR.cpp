/* Copyright (c) 2008-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/BFSODPOR.hpp"
#include "src/mc/api/states/BFSWutState.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_bfsodpor, mc_reduction, "Logging specific to the odpor reduction");

namespace simgrid::mc {

void BFSODPOR::races_computation(odpor::Execution& E, stack_t* S, std::vector<std::shared_ptr<State>>* opened_states)
{
  State* s = S->back().get();
  // ODPOR only look for race on the maximal executions
  if (not s->get_enabled_actors().empty()) {
    auto s_cast = static_cast<WutState*>(S->back().get());
    xbt_assert(s != nullptr, "ODPOR should use WutState. Fix me");
    if (s_cast->direct_children() > 1 and opened_states != nullptr)
      opened_states->emplace_back(s);
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
      WutState* prev_state = static_cast<WutState*>((*S)[e].get());
      xbt_assert(prev_state != nullptr, "ODPOR should use WutState. Fix me");

      if (const auto v = E.get_odpor_extension_from(e, e_prime, *prev_state); v.has_value())
        prev_state->insert_into_wakeup_tree(v.value());
    }
    E.get_event_with_handle(e_prime).consider_races();
  }
}

aid_t BFSODPOR::next_to_explore(odpor::Execution& E, stack_t* S)
{
  auto s = static_cast<BFSWutState*>(S->back().get());
  xbt_assert(s != nullptr, "BFSODPOR should use BFSWutState. Fix me");
  const aid_t next = s->next_to_explore();

  if (next == -1)
    return -1;

  if (not s->is_actor_enabled(next)) {
    xbt_die("Can BFSDPOR execute disabled transition? Fix Me");
    XBT_DEBUG("ODPOR wants to execute a disabled transition %s.",
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
        existing_state != nullptr)
      return existing_state;
    return std::make_shared<BFSWutState>(remote_app, bfswut_state);
  }
}

} // namespace simgrid::mc
