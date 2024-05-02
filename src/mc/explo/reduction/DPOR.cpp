
/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/reduction/DPOR.hpp"

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

void DPOR::races_computation(odpor::Execution& E, stack_t* S, std::vector<StatePtr>* opened_states)
{
  // If there are less then 2 events, there is no possible race yet
  if (E.size() <= 1)
    return;

  State* s = S->back().get();

  // With persistency, we only need to test the race detection for the lastly taken proc
  aid_t proc                     = s->get_transition_in()->aid_;
  odpor::Execution E_before_last = E.get_prefix_before(E.size() - 1);

  if (std::optional<EventHandle> opt_i = max_dependent_dpor(E_before_last, s, proc); opt_i.has_value()) {
    EventHandle i                       = opt_i.value();
    std::unordered_set<aid_t> ancestors = compute_ancestors(E_before_last, S, proc, i);
    // note: computing the whole set is necessary with guiding strategy
    if (not ancestors.empty()) {
      (*S)[i]->ensure_one_considered_among_set(ancestors);
    } else {
      (*S)[i]->consider_all();
    }
    if (opened_states != nullptr)
      opened_states->emplace_back((*S)[i]);
  }
}

StatePtr DPOR::state_create(RemoteApp& remote_app, StatePtr parent_state)
{
  auto res             = Reduction::state_create(remote_app, parent_state);
  auto sleep_set_state = static_cast<SleepSetState*>(res.get());

  if (not sleep_set_state->get_enabled_minus_sleep().empty()) {
    sleep_set_state->consider_best();
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
