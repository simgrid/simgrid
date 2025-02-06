/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/DPOR.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <cstddef>
#include <memory>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dpor, mc, "Dynamic partial order reduction algorithm");

namespace simgrid::mc {

std::optional<EventHandle> DPOR::max_dependent_dpor(const odpor::Execution& S, const State* s, aid_t p)
{

  if (S.size() == 0)
    return {};

  for (EventHandle i = S.size(); i > 0; i--) {
    auto past_transition      = S.get_transition_for_handle(i - 1);
    auto next_transition_of_p = s->get_transition_in();

    if (past_transition->depends(next_transition_of_p.get()) &&
        past_transition->can_be_co_enabled(next_transition_of_p.get()) && not S.happens_before_process(i - 1, p))
      return i - 1;
  }

  return {};
}

std::unordered_set<aid_t> DPOR::compute_ancestors(const odpor::Execution& S, stack_t* state_stack, aid_t p,
                                                  EventHandle i)
{

  XBT_DEBUG("Computing the ancestors of aid %ld before event %u", p, i);
  std::unordered_set<aid_t> E = std::unordered_set<aid_t>();
  for (aid_t q : (*state_stack)[i]->get_enabled_actors()) {

    if (q == p) {
      E.insert(q);
      continue;
    }

    for (EventHandle j = i + 1; j < S.size() - 1; j++) {
      if (q == S.get_actor_with_handle(j) && S.happens_before_process(j, p)) {
        E.insert(q);
        break;
      }
    }
  }

  return E;
}

std::shared_ptr<Reduction::RaceUpdate> DPOR::races_computation(odpor::Execution& E, stack_t* S,
                                                               std::vector<StatePtr>* opened_states)
{
  XBT_DEBUG("Doing the race computation phase with a stack of size %lu and an execution of size %lu", S->size(),
            E.size());

  State* last_state = S->back().get();
  // let's look for all the races but only at the end
  if (not last_state->get_enabled_actors().empty())
    return std::make_shared<RaceUpdate>();

  auto updates = std::make_shared<RaceUpdate>();

  for (std::size_t i = 1; i < S->size(); i++) {
    StatePtr s = (*S)[i];
    // With persistency, we only need to test the race detection for the lastly taken proc
    aid_t proc                     = s->get_transition_in()->aid_;
    odpor::Execution E_before_last = E.get_prefix_before(i - 1);
    if (std::optional<EventHandle> opt_j = max_dependent_dpor(E_before_last, s.get(), proc); opt_j.has_value()) {
      EventHandle j                       = opt_j.value();
      std::unordered_set<aid_t> ancestors = compute_ancestors(E_before_last, S, proc, j);
      // note: computing the whole set is necessary with guiding strategy

      updates->add_element((*S)[j], std::move(ancestors));
    }
  }
  return updates;
}

unsigned long DPOR::apply_race_update(std::shared_ptr<Reduction::RaceUpdate> updates,
                                      std::vector<StatePtr>* opened_states)
{

  auto dpor_updates        = static_cast<RaceUpdate*>(updates.get());
  unsigned long nb_updates = 0;
  for (auto& [state, ancestors] : dpor_updates->get_value()) {
    if (not ancestors.empty()) {
      Exploration::get_strategy()->ensure_one_considered_among_set_in(state.get(), ancestors);
    } else {
      state->consider_all();
    }
    if (opened_states != nullptr) {
      opened_states->emplace_back(state);
      nb_updates++;
    }
  }
  return nb_updates;
}

StatePtr DPOR::state_create(RemoteApp& remote_app, StatePtr parent_state)
{
  auto res             = Reduction::state_create(remote_app, parent_state);
  auto sleep_set_state = static_cast<SleepSetState*>(res.get());

  if (not sleep_set_state->get_enabled_minus_sleep().empty()) {
    Exploration::get_strategy()->consider_best_in(sleep_set_state);
  }

  return res;
}

aid_t DPOR::next_to_explore(odpor::Execution& E, stack_t* S)
{
  if (S->back()->get_batrack_minus_done().empty())
    return -1;
  return S->back()->next_transition_guided().first;
}

} // namespace simgrid::mc
