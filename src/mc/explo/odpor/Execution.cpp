/* Copyright (c) 2008-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"
#include <algorithm>
#include <limits>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_odpor_execution, mc_dfs, "ODPOR exploration algorithm of the model-checker");

namespace simgrid::mc::odpor {

std::vector<std::string> get_textual_trace(const PartialExecution& w)
{
  std::vector<std::string> trace;
  for (const auto& t : w) {
    auto a = xbt::string_printf("Actor %ld: %s", t->aid_, t->to_string(true).c_str());
    trace.emplace_back(std::move(a));
  }
  return trace;
}
std::string one_string_textual_trace(const PartialExecution& w)
{
  std::vector<std::string> trace = get_textual_trace(w);
  std::string res;
  for (const auto& one_actor : trace) {
    res += one_actor;
    res += "\n";
  }
  return res;
}

Execution::Execution(const PartialExecution& w)
{
  push_partial_execution(w);
}

void Execution::push_transition(std::shared_ptr<Transition> t)
{
  xbt_assert(t != nullptr, "Unexpectedly received `nullptr`");

  ClockVector max_clock_vector;
  for (const Event& e : this->contents_) {
    if (e.get_transition()->depends(t.get())) {
      ClockVector::max_emplace_left(max_clock_vector, e.get_clock_vector());
    }
  }
  max_clock_vector[t->aid_] = this->size();
  contents_.push_back(Event({std::move(t), std::move(max_clock_vector)}));
}

void Execution::remove_last_event()
{
  xbt_assert(!contents_.empty(), "Tried to remove an element from an empty Execution");
  contents_.pop_back();
}

void Execution::push_partial_execution(const PartialExecution& w)
{
  for (const auto& t : w) {
    push_transition(t);
  }
}

std::vector<std::string> Execution::get_textual_trace() const
{
  std::vector<std::string> trace;
  for (EventHandle e_i = 0; e_i != this->contents_.size(); e_i++) {
    auto a = xbt::string_printf("Actor %ld: %s (Is racy = %s)", contents_[e_i].get_transition()->aid_,
                                contents_[e_i].get_transition()->to_string(true).c_str(),
                                get_racing_events_of(e_i).empty() ? "Yes" : "No");

    trace.emplace_back(std::move(a));
  }
  return trace;
}

std::list<Execution::EventHandle> Execution::get_racing_events_of(Execution::EventHandle target) const
{
  std::list<Execution::EventHandle> racing_events;
  std::list<Execution::EventHandle> candidates;
  for (auto const& [aid, event_handle] : get_event_with_handle(target).get_clock_vector())
    if (aid != get_actor_with_handle(target))
      candidates.push_back(event_handle);

  candidates.sort(std::greater<EventHandle>());
  candidates.unique();

  // We need to determine previous action of actor proc(target) in order
  // to fully determine if there exist a "event in the middle"
  Execution::EventHandle prev_on_actor;
  for (prev_on_actor = target - 1; prev_on_actor != std::numeric_limits<Execution::EventHandle>::max(); prev_on_actor--)
    if (get_actor_with_handle(prev_on_actor) == get_actor_with_handle(target))
      break;

  bool disqualified;
  // For each event in the vector clock that is not on the event itself
  for (auto e_i : candidates) {
    if (prev_on_actor != std::numeric_limits<Execution::EventHandle>::max() and happens_before(e_i, prev_on_actor))
      continue;

    disqualified = false;
    // If there exist an event e_j such that e_i --> e_j --> target
    // then e_j was found in the candidates already and we must drop e_i
    for (auto e_j : racing_events) {

      if (happens_before(e_i, e_j)) {
        disqualified = true;
        break;
      }
    }
    if (disqualified)
      continue;

    XBT_DEBUG("ODPOR_RACING_EVENTS with `%u` (%s) : `%u` (%s) is a valid racing event", target,
              get_transition_for_handle(target)->to_string().c_str(), e_i,
              get_transition_for_handle(e_i)->to_string().c_str());
    racing_events.push_back(e_i);
  }

  return racing_events;
}

std::list<Execution::EventHandle> Execution::get_reversible_races_of(EventHandle handle) const
{
  std::list<EventHandle> reversible_races;
  const auto* this_transition = get_transition_for_handle(handle);
  for (EventHandle race : get_racing_events_of(handle)) {
    const auto* other_transition = get_transition_for_handle(race);

    if (this_transition->reversible_race(other_transition)) {
      reversible_races.push_back(race);
    }
  }
  return reversible_races;
}

Execution Execution::get_prefix_before(Execution::EventHandle handle) const
{
  return Execution(std::vector<Event>{contents_.begin(), contents_.begin() + handle});
}

std::unordered_set<aid_t>
Execution::get_missing_source_set_actors_from(EventHandle e, const std::unordered_set<aid_t>& backtrack_set) const
{
  // If this execution is empty, there are no initials
  // relative to the last transition added to the execution
  // since such a transition does not exist
  if (empty()) {
    return std::unordered_set<aid_t>{};
  }

  // To actually compute `I_[E'](v) ∩ backtrack(E')`, we must
  // first compute `E'` and "move" in the direction of `v`.
  // We perform a scan over `E` (this execution) and make
  // note of any events which occur after `e` but don't
  // "happen-after" `e` by pushing them onto `E'`. Note that
  // correctness is still preserved in computing `v` "on-the-fly"
  // to determine if an event `e` by actor `q` is an initial for `E'`
  // after `v`: only those events that "occur-before" `e` in `v` could
  // "happen-before" `ve for any valid "happens-before" relation
  // (see property 1 in the ODPOR paper, viz. "is included in <_E")

  // First, grab `E' := pre(e, E)` and determine what actor `p` is
  const auto next_E_p = get_latest_event_handle().value();
  xbt_assert(e != next_E_p,
             "This method assumes that the event `e` (%u) and `next_[E](p)` (%u)"
             "are in a reversible race, yet we claim to have such a race between the"
             "same event. This indicates the the SDPOR pseudocode implementation is broken "
             "as it supplies these values.",
             e, next_E_p);
  Execution E_prime_v = get_prefix_before(e);
  std::vector<sdpor::Execution::EventHandle> v;
  std::unordered_set<aid_t> I_E_prime_v;
  std::unordered_set<aid_t> disqualified_actors;

  // Note `e + 1` here: `notdep(e, E)` is defined as the
  // set of events that *occur-after* but don't *happen-after* `e`
  for (auto e_prime = e + 1; e_prime <= next_E_p; ++e_prime) {
    // Any event `e*` which occurs after `e` but which does not
    // happen after `e` is a member of `v`. In addition to marking
    // the event in `v`, we also "simulate" running the action `v`
    // from E'
    if (not happens_before(e, e_prime) || e_prime == next_E_p) {
      // First, push the transition onto the hypothetical execution
      E_prime_v.push_transition(get_event_with_handle(e_prime).get_transition());
      const EventHandle e_prime_in_E_prime_v = E_prime_v.get_latest_event_handle().value();

      // When checking whether any event in `dom_[E'](v)` happens before
      // `next_[E'](q)` below for thread `q`, we must consider that the
      // events relative to `E` (this execution) are different than those
      // relative to `E'.v`. Thus e.g. event `7` in `E` may be event `4`
      // in `E'.v`. Since we are asking about "happens-before"
      // `-->_[E'.v]` about `E'.v`, we must build `v` relative to `E'`.
      //
      // Note that we add `q` to v regardless of whether `q` itself has been
      // disqualified since  we've determined that `e_prime` "occurs-after" but
      // does not "happen-after" `e`
      v.push_back(e_prime_in_E_prime_v);

      const aid_t q = E_prime_v.get_actor_with_handle(e_prime_in_E_prime_v);
      if (disqualified_actors.count(q) > 0) { // Did we already note that `q` is not an initial?
        continue;
      }
      const bool is_initial = std::none_of(v.begin(), v.end(), [&](const auto& e_star) {
        return E_prime_v.happens_before(e_star, e_prime_in_E_prime_v);
      });
      if (is_initial) {
        // If the backtrack set already contains `q`, we're done:
        // they've made note to search for (or have already searched for)
        // this initial
        if (backtrack_set.count(q) > 0) {
          return std::unordered_set<aid_t>{};
        } else {
          I_E_prime_v.insert(q);
        }
      } else {
        // If `q` is disqualified as a candidate, clearly
        // no event occurring after `e_prime` in `E` executed
        // by actor `q` will qualify since any (valid) happens-before
        // relation orders actions taken by each actor
        disqualified_actors.insert(q);
      }
    }
  }
  xbt_assert(not I_E_prime_v.empty(),
             "For any non-empty execution, we know that "
             "at minimum one actor is an initial since "
             "some execution is possible with respect to a "
             "prefix before event `%u`, yet we didn't find anyone. "
             "This implies the implementation of this function is broken.",
             e);
  return I_E_prime_v;
}

std::optional<PartialExecution> Execution::get_odpor_extension_from(EventHandle e, EventHandle e_prime,
                                                                    const SleepSetState& state_at_e) const
{
  // `e` is assumed to be in a reversible race with `e_prime`.
  // If `e > e_prime`, then `e` occurs-after `e_prime` which means
  // `e` could not race with if
  if (e > e_prime) {
    throw std::invalid_argument("ODPOR extensions can only be computed for "
                                "events in a reversible race, which is claimed, "
                                "yet the racing event 'occurs-after' the target");
  }

  if (empty()) {
    return std::nullopt;
  }

  PartialExecution v;
  std::unordered_set<aid_t> disqualified_actors = {get_actor_with_handle(e)};
  const std::unordered_set<aid_t> sleep_E_prime = state_at_e.get_sleeping_actors();

  // For each event after e, find the first dependent on each actor. From this point,
  // all other event on those actors "happens-after" and are then disqualified from
  // the construction of v.
  for (auto e_star = e + 1; e_star <= get_latest_event_handle().value(); ++e_star) {

    if (disqualified_actors.count(get_actor_with_handle(e_star)) > 0)
      continue;

    if (happens_before(e, e_star)) {
      disqualified_actors.emplace(get_actor_with_handle(e_star));
      continue;
    }

    xbt_assert(e_star != e_prime,
               "Invariant Violation: We claimed events %u and %u were in a reversible race, yet we also "
               "claim that they do not happen-before one another. This is impossible: "
               "are you sure that the two events are in a reversible race?",
               e, e_prime);

    v.push_back(get_event_with_handle(e_star).get_transition());
  }

  v.push_back(get_event_with_handle(e_prime).get_transition());

  XBT_DEBUG("Potential v := \n%s", one_string_textual_trace(v).c_str());

  for (auto transition_it = v.begin(); transition_it != v.end(); ++transition_it) {
    const bool is_initial = std::none_of(v.begin(), transition_it, [&](const auto& transition_it_prime) {
      return (*transition_it)->depends(transition_it_prime.get());
    });
    if (is_initial) {
      // If the sleep set already contains `q`, we're done:
      // we've found an initial contained in the sleep set and
      // so the intersection is non-empty
      if (sleep_E_prime.count((*transition_it)->aid_) > 0) {
        return std::nullopt;
      }
    }
  }

  for (const auto& [aid, astate] : state_at_e.get_actors_list()) {
    const bool is_in_WI_E      = astate.is_enabled() and is_independent_with_execution_of(v, astate.get_transition());
    const bool is_in_sleep_set = sleep_E_prime.count(aid) > 0;

    // `action(aid)` is in `WI_[E](v)` but also is contained in the sleep set.
    // This implies that the intersection between the two is non-empty
    if (is_in_WI_E && is_in_sleep_set)
      return std::nullopt;
  }

  return v;
}

bool Execution::is_initial_after_execution_of(const PartialExecution& w, aid_t p)
{

  for (auto w_i = w.begin(); w_i != w.end(); w_i++) {
    if ((*w_i)->aid_ != p)
      continue;

    for (auto w_j = w.begin(); w_j != w_i; w_j++) {
      if ((*w_j)->depends((*w_i).get()))
        return false;
    }

    return true;
  }

  return false;
}

bool Execution::is_independent_with_execution_of(const PartialExecution& w, std::shared_ptr<Transition> next_E_p)
{
  for (const auto& transition : w)
    if (transition->depends(next_E_p.get()))
      return false;

  return true;
}

std::optional<PartialExecution> Execution::get_shortest_odpor_sq_subset_insertion(const PartialExecution& v,
                                                                                  const PartialExecution& w)
{
  // See section 4 of Abdulla. et al.'s 2017 ODPOR paper for details (specifically
  // where the [iterative] computation of `v ~_[E] w` is described)
  auto w_now = w;
  XBT_DEBUG("Computing 'v~_[E]w' with v:=\n%s w:=\n%s", one_string_textual_trace(v).c_str(),
            one_string_textual_trace(w).c_str());
  for (const auto& next_E_p : v) {
    // Is `p in `I_[E](w)`?

    if (const aid_t p = next_E_p->aid_; is_initial_after_execution_of(w_now, p)) {
      // Remove `p` from w and continue

      // INVARIANT: If `p` occurs in `w`, it had better refer to the same
      // transition referenced by `v`. Unfortunately, we have two
      // sources of truth here which can be manipulated at the same
      // time as arguments to the function. If ODPOR works correctly,
      // they should always refer to the same value; but as a sanity check,
      // we have an assert that tests that at least the types are the same.
      const auto action_by_p_in_w =
          std::find_if(w_now.begin(), w_now.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w != w_now.end(), "Invariant violated: actor `p` "
                                                  "is claimed to be an initial after `w` but is "
                                                  "not actually contained in `w`. This indicates that there "
                                                  "is a bug computing initials");
      const auto& w_action = *action_by_p_in_w;
      xbt_assert(w_action->type_ == next_E_p->type_,
                 "Invariant violated: `v` claims that actor `%ld` executes '%s' while "
                 "`w` claims that it executes '%s'. These two partial executions both "
                 "refer to `next_[E](p)`, which should be the same",
                 p, next_E_p->to_string(false).c_str(), w_action->to_string(false).c_str());
      w_now.erase(action_by_p_in_w);
    }
    // Is `E ⊢ p ◇ w`?
    else if (is_independent_with_execution_of(w_now, next_E_p)) {
      // INVARIANT: Note that it is impossible for `p` to be
      // excluded from the set `I_[E](w)` BUT ALSO be contained in
      // `w` itself if `E ⊢ p ◇ w` (intuitively, the fact that `E ⊢ p ◇ w`
      // means that are able to move `p` anywhere in `w` IF it occurred, so
      // if it really does occur we know it must then be an initial).
      // We assert this is the case here
      const auto action_by_p_in_w =
          std::find_if(w_now.begin(), w_now.end(), [=](const auto& action) { return action->aid_ == p; });
      xbt_assert(action_by_p_in_w == w_now.end(),
                 "Invariant violated: We claimed that actor `%ld` is not an initial "
                 "after `w`, yet it's independent with all actions of `w` AND occurs in `w`."
                 "This indicates that there is a bug computing initials",
                 p);
    } else {
      // Neither of the two above conditions hold, so the relation fails
      return std::nullopt;
    }
  }
  return std::optional<PartialExecution>{std::move(w_now)};
}

bool Execution::happens_before_process(Execution::EventHandle e, aid_t p) const
{

  if (get_actor_with_handle(e) == p)
    return true;

  for (EventHandle k = e + 1; k < contents_.size(); k++) {
    if (happens_before(e, k) && get_actor_with_handle(k) == p)
      return true;
  }
  return false;
}

bool Execution::happens_before(Execution::EventHandle e1_handle, Execution::EventHandle e2_handle) const
{
  // 1. "happens-before" (-->_E) is a subset of "occurs before" (<_E)
  // and is an irreflexive relation
  if (e1_handle >= e2_handle) {
    return false;
  }

  // Each execution maintains a stack of clock vectors which are updated
  // according to the procedure outlined in section 4 of the original DPOR paper
  const Event& e2     = get_event_with_handle(e2_handle);
  const aid_t proc_e1 = get_actor_with_handle(e1_handle);

  if (const auto e1_in_e2_clock = e2.get_clock_vector().get(proc_e1); e1_in_e2_clock.has_value()) {
    return e1_handle <= e1_in_e2_clock.value();
  }
  // If `e1` does not appear in e2's clock vector, this implies
  // not only that the transitions associated with `e1` and `e2
  // are independent, but further that there are no transitive
  // dependencies between e1 and e2
  return false;
}

} // namespace simgrid::mc::odpor
