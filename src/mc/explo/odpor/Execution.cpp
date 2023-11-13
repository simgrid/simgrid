/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/api/State.hpp"
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

Execution::Execution(const PartialExecution& w)
{
  push_partial_execution(w);
}

void Execution::push_transition(std::shared_ptr<Transition> t)
{
  if (t == nullptr) {
    throw std::invalid_argument("Unexpectedly received `nullptr`");
  }
  ClockVector max_clock_vector;
  for (const Event& e : this->contents_) {
    if (e.get_transition()->depends(t.get())) {
      max_clock_vector = ClockVector::max(max_clock_vector, e.get_clock_vector());
    }
  }
  max_clock_vector[t->aid_] = this->size();
  contents_.push_back(Event({std::move(t), max_clock_vector}));
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
  for (const auto& t : this->contents_) {
    auto a = xbt::string_printf("Actor %ld: %s", t.get_transition()->aid_, t.get_transition()->to_string(true).c_str());
    trace.emplace_back(std::move(a));
  }
  return trace;
}

std::unordered_set<Execution::EventHandle> Execution::get_racing_events_of(Execution::EventHandle target) const
{
  std::unordered_set<Execution::EventHandle> racing_events;
  // This keep tracks of events that happens-before the target
  std::unordered_set<Execution::EventHandle> disqualified_events;

  // For each event of the execution
  for (auto e_i = target; e_i != std::numeric_limits<Execution::EventHandle>::max(); e_i--) {
    // We need `e_i -->_E target` as a necessary condition
    if (not happens_before(e_i, target)) {
      XBT_DEBUG("ODPOR_RACING_EVENTS with `%u` : `%u` discarded because `%u` --\\-->_E `%u`", target, e_i, e_i, target);
      continue;
    }

    // Further, `proc(e_i) != proc(target)`
    if (get_actor_with_handle(e_i) == get_actor_with_handle(target)) {
      disqualified_events.insert(e_i);
      XBT_DEBUG("ODPOR_RACING_EVENTS with `%u` : `%u` disqualified because proc(`%u`)=proc(`%u`)", target, e_i, e_i,
                target);
      continue;
    }

    // There could an event that "happens-between" the two events which would discount `e_i` as a race
    for (auto e_j = e_i; e_j < target; e_j++) {
      // If both:
      // 1. e_i --->_E e_j; and
      // 2. disqualified_events.count(e_j) > 0
      // then e_i --->_E target indirectly (either through
      // e_j directly, or transitively through e_j)
      if (disqualified_events.count(e_j) > 0 && happens_before(e_i, e_j)) {
        XBT_DEBUG("ODPOR_RACING_EVENTS with `%u` : `%u` disqualified because `%u` happens-between `%u`-->`%u`-->`%u`)",
                  target, e_i, e_j, e_i, e_j, target);
        disqualified_events.insert(e_i);
        break;
      }
    }

    // If `e_i` wasn't disqualified in the last round,
    // it's in a race with `target`. After marking it
    // as such, we ensure no other event `e` can happen-before
    // it (since this would transitively make it the event
    // which "happens-between" `target` and `e`)
    if (disqualified_events.count(e_i) == 0) {
      XBT_DEBUG("ODPOR_RACING_EVENTS with `%u` : `%u` is a valid racing event", target, e_i);
      racing_events.insert(e_i);
      disqualified_events.insert(e_i);
    }
  }

  return racing_events;
}

std::unordered_set<Execution::EventHandle> Execution::get_reversible_races_of(EventHandle handle) const
{
  std::unordered_set<EventHandle> reversible_races;
  const auto* this_transition = get_transition_for_handle(handle);
  for (EventHandle race : get_racing_events_of(handle)) {
    const auto* other_transition = get_transition_for_handle(race);

    if (this_transition->reversible_race(other_transition)) {
      reversible_races.insert(race);
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
                                                                    const State& state_at_e) const
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
  std::vector<Execution::EventHandle> v_handles;
  std::unordered_set<aid_t> WI_E_prime_v;
  std::unordered_set<aid_t> disqualified_actors;
  Execution E_prime_v                           = get_prefix_before(e);
  const std::unordered_set<aid_t> sleep_E_prime = state_at_e.get_sleeping_actors();

  // Note `e + 1` here: `notdep(e, E)` is defined as the
  // set of events that *occur-after* but don't *happen-after* `e`
  //
  // SUBTLE NOTE: ODPOR requires us to compute `notdep(e, E)` EVEN THOUGH
  // the race is between `e` and `e'`; that is, events occurring in `E`
  // that "occur-after" `e'` may end up in the partial execution `v`.
  //
  // Observe that `notdep(e, E).proc(e')` will contain all transitions
  // that don't happen-after `e` in the order they appear FOLLOWED BY
  // THE **TRANSITION** ASSOCIATED WITH **`e'`**!!
  //
  // SUBTLE NOTE: Observe that any event that "happens-after" `e'`
  // must necessarily "happen-after" `e` as well, since `e` and
  // `e'` are presumed to be in a reversible race. Hence, we know that
  // all events `e_star` such that `e` "happens-before" `e_star` cannot affect
  // the enabledness of `e'`; furthermore, `e'` cannot affect the enabledness
  // of any event independent with `e` that "occurs-after" `e'`
  for (auto e_star = e + 1; e_star <= get_latest_event_handle().value(); ++e_star) {
    // Any event `e*` which occurs after `e` but which does not
    // happen after `e` is a member of `v`. In addition to marking
    // the event in `v`, we also "simulate" running the action `v` from E'
    // to be able to compute `--->[E'.v]`
    if (not happens_before(e, e_star)) {
      xbt_assert(e_star != e_prime,
                 "Invariant Violation: We claimed events %u and %u were in a reversible race, yet we also "
                 "claim that they do not happen-before one another. This is impossible: "
                 "are you sure that the two events are in a reversible race?",
                 e, e_prime);
      E_prime_v.push_transition(get_event_with_handle(e_star).get_transition());
      v.push_back(get_event_with_handle(e_star).get_transition());

      XBT_DEBUG("Added Event `%u` (%ld:%s) to the construction of v", e_star, get_actor_with_handle(e_star),
                get_event_with_handle(e_star).get_transition()->to_string().c_str());

      const EventHandle e_star_in_E_prime_v = E_prime_v.get_latest_event_handle().value();

      // When checking whether any event in `dom_[E'](v)` happens before
      // `next_[E'](q)` below for thread `q`, we must consider that the
      // events relative to `E` (this execution) are different than those
      // relative to `E'.v`. Thus e.g. event `7` in `E` may be event `4`
      // in `E'.v`. Since we are asking about "happens-before"
      // `-->_[E'.v]` about `E'.v`, we must build `v` relative to `E'`
      v_handles.push_back(e_star_in_E_prime_v);

      // Note that we add `q` to v regardless of whether `q` itself has been
      // disqualified since `q` may itself disqualify other actors
      // (i.e. even if `q` is disqualified from being an initial, it
      // is still contained in the sequence `v`)
      const aid_t q = E_prime_v.get_actor_with_handle(e_star_in_E_prime_v);
      if (disqualified_actors.count(q) > 0) { // Did we already note that `q` is not an initial?
        continue;
      }
      const bool is_initial = std::none_of(v_handles.begin(), v_handles.end(), [&](const auto& handle) {
        return E_prime_v.happens_before(handle, e_star_in_E_prime_v);
      });
      if (is_initial) {
        // If the sleep set already contains `q`, we're done:
        // we've found an initial contained in the sleep set and
        // so the intersection is non-empty
        if (sleep_E_prime.count(q) > 0) {
          return std::nullopt;
        } else {
          WI_E_prime_v.insert(q);
        }
      } else {
        // If `q` is disqualified as a candidate, clearly
        // no event occurring after `e_prime` in `E` executed
        // by actor `q` will qualify since any (valid) happens-before
        // relation orders actions taken by each actor
        disqualified_actors.insert(q);
      }
    } else {
      XBT_DEBUG("Event `%u` (%ld:%s) dismissed from the construction of v", e_star, get_actor_with_handle(e_star),
                get_event_with_handle(e_star).get_transition()->to_string().c_str());
    }
  }

  // Now we add `e_prime := <q, i>` to `E'.v` and repeat the same work
  // It's possible `proc(e_prime)` is an initial
  //
  // Note the form of `v` in the pseudocode:
  //  `v := notdep(e, E).e'^
  E_prime_v.push_transition(get_event_with_handle(e_prime).get_transition());
  v.push_back(get_event_with_handle(e_prime).get_transition());

  const EventHandle e_prime_in_E_prime_v = E_prime_v.get_latest_event_handle().value();
  v_handles.push_back(e_prime_in_E_prime_v);

  const bool is_initial = std::none_of(v_handles.begin(), v_handles.end(), [&](const auto& handle) {
    return E_prime_v.happens_before(handle, e_prime_in_E_prime_v);
  });
  if (is_initial) {
    if (const aid_t q = E_prime_v.get_actor_with_handle(e_prime_in_E_prime_v); sleep_E_prime.count(q) > 0) {
      return std::nullopt;
    } else {
      WI_E_prime_v.insert(q);
    }
  }

  const Execution pre_E_e    = get_prefix_before(e);
  const auto sleeping_actors = state_at_e.get_sleeping_actors();

  // Check if any enabled actor that is independent with
  // this execution after `v` is contained in the sleep set
  for (const auto& [aid, astate] : state_at_e.get_actors_list()) {
    const bool is_in_WI_E =
        astate.is_enabled() and pre_E_e.is_independent_with_execution_of(v, astate.get_transition());
    const bool is_in_sleep_set = sleeping_actors.count(aid) > 0;

    // `action(aid)` is in `WI_[E](v)` but also is contained in the sleep set.
    // This implies that the intersection between the two is non-empty
    if (is_in_WI_E && is_in_sleep_set)
      return std::nullopt;
  }

  return v;
}

bool Execution::is_initial_after_execution_of(const PartialExecution& w, aid_t p) const
{
  auto E_w = *this;
  std::vector<EventHandle> w_handles;
  for (const auto& w_i : w) {
    // Take one step in the direction of `w`
    E_w.push_transition(w_i);

    // If that step happened to be executed by `p`,
    // great: we know that `p` is contained in `w`.
    // We now need to verify that it doens't "happen-after"
    // any events which occur before it
    if (w_i->aid_ == p) {
      const auto p_handle = E_w.get_latest_event_handle().value();
      return std::none_of(w_handles.begin(), w_handles.end(),
                          [&](const auto handle) { return E_w.happens_before(handle, p_handle); });
    } else {
      w_handles.push_back(E_w.get_latest_event_handle().value());
    }
  }
  return false;
}

bool Execution::is_independent_with_execution_of(const PartialExecution& w, std::shared_ptr<Transition> next_E_p) const
{
  // INVARIANT: Here, we assume that for any process `p_i` of `w`,
  // the events of this execution followed by the execution of all
  // actors occurring before `p_i` in `v` (`p_j`, `0 <= j < i`)
  // are sufficient to enable `p_i`. This is fortunately the case
  // with what ODPOR requires of us, viz. to ask the question about
  // `v := notdep(e, E)` for some execution `E` and event `e` of
  // that execution.
  auto E_p_w = *this;
  E_p_w.push_transition(std::move(next_E_p));
  const auto p_handle = E_p_w.get_latest_event_handle().value();

  // As we add events to `w`, verify that none
  // of them "happen-after" the event associated with
  // the step `next_E_p` (viz. p_handle)
  for (const auto& w_i : w) {
    E_p_w.push_transition(w_i);
    const auto w_i_handle = E_p_w.get_latest_event_handle().value();
    if (E_p_w.happens_before(p_handle, w_i_handle)) {
      return false;
    }
  }
  return true;
}

std::optional<PartialExecution> Execution::get_shortest_odpor_sq_subset_insertion(const PartialExecution& v,
                                                                                  const PartialExecution& w) const
{
  // See section 4 of Abdulla. et al.'s 2017 ODPOR paper for details (specifically
  // where the [iterative] computation of `v ~_[E] w` is described)
  auto E_v   = *this;
  auto w_now = w;

  for (const auto& next_E_p : v) {
    // Is `p in `I_[E](w)`?
    if (const aid_t p = next_E_p->aid_; E_v.is_initial_after_execution_of(w_now, p)) {
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
    else if (E_v.is_independent_with_execution_of(w_now, next_E_p)) {
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

    // Move one step forward in the direction of `v` and repeat
    E_v.push_transition(next_E_p);
  }
  return std::optional<PartialExecution>{std::move(w_now)};
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
