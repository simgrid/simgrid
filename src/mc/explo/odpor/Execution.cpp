/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/Execution.hpp"
#include <exception>
#include <limits>

namespace simgrid::mc::odpor {

void Execution::push_transition(const Transition* t)
{
  if (t == nullptr) {
    throw std::invalid_argument("Unexpectedly received `nullptr`");
  }
  ClockVector max_clock_vector;
  for (const Event& e : this->contents_) {
    if (e.get_transition()->depends(t)) {
      max_clock_vector = ClockVector::max(max_clock_vector, e.get_clock_vector());
    }
  }
  max_clock_vector[t->aid_] = this->size();
  contents_.push_back(Event({t, max_clock_vector}));
}

std::unordered_set<Execution::EventHandle> Execution::get_racing_events_of(Execution::EventHandle target) const
{
  std::unordered_set<Execution::EventHandle> racing_events;
  std::unordered_set<Execution::EventHandle> disqualified_events;

  // For each event of the execution
  for (auto e_i = target; e_i != std::numeric_limits<Execution::EventHandle>::max(); e_i--) {
    // We need `e_i -->_E target` as a necessary condition
    if (not happens_before(e_i, target)) {
      continue;
    }

    // Further, `proc(e_i) != proc(target)`
    if (get_actor_with_handle(e_i) == get_actor_with_handle(target)) {
      disqualified_events.insert(e_i);
      continue;
    }

    // There could an event that "happens-between" the two events which would discount `e_i` as a race
    for (auto e_j = e_i; e_j < target; e_j++) {
      // If both:
      // 1. e_i --->_E e_j; and
      // 2. disqualified_events.count(e_j) > 0
      // then e_i --->_E target indirectly (either through
      // e_j directly, or transitively through e_j)
      if (happens_before(e_i, e_j) and disqualified_events.count(e_j) > 0) {
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
      racing_events.insert(e_i);
      disqualified_events.insert(e_i);
    }
  }

  return racing_events;
}

Execution Execution::get_prefix_up_to(Execution::EventHandle handle) const
{
  return Execution(std::vector<Event>{contents_.begin(), contents_.begin() + handle});
}

std::optional<aid_t> Execution::get_first_sdpor_initial_from(EventHandle e,
                                                             std::unordered_set<aid_t> disqualified_actors) const
{
  // If this execution is empty, there are no initials
  // relative to the last transition added to the execution
  // since such a transition does not exist
  if (empty()) {
    return std::nullopt;
  }

  // To actually compute `I_[E'](v) ∩ backtrack(E')`, we must
  // first compute `E'` and "move" in the direction of `v`.
  // We perform a scan over `E` (this execution) and make
  // note of any events which occur after `e` but don't
  // "happen-after" `e` by pushing them onto `E'`. Note that
  // correctness is still preserved in computing `v` "on-the-fly"
  // to determine if an actor `q` is an initial for `E'` after `v`:
  // only those events that "occur-before" `v`
  // could happen-before `v` for any valid happens-before relation.

  // First, grab `E' := pre(e, E)` and determine what actor `p` is
  // TODO: Instead of copying around these big structs, it
  // would behoove us to incorporate some way to reference
  // portions of an execution. For simplicity and for a
  // "proof of concept" version, we opt to simply copy
  // the contents instead of making a view into the execution
  const auto next_E_p = get_latest_event_handle().value();
  Execution E_prime_v = get_prefix_up_to(e);
  std::vector<sdpor::Execution::EventHandle> v;

  // Note `e + 1` here: `notdep(e, E)` is defined as the
  // set of events that *occur-after* but don't *happen-after* `e`
  for (auto e_prime = e + 1; e_prime <= next_E_p; ++e_prime) {
    // Any event `e*` which occurs after `e` but which does not
    // happen after `e` is a member of `v`. In addition to marking
    // the event in `v`, we also "simulate" running the action `v`
    // from E'
    if (not happens_before(e, e_prime) or e_prime == next_E_p) {
      // First, push the transition onto the hypothetical execution
      E_prime_v.push_transition(get_event_with_handle(e_prime).get_transition());
      const EventHandle e_prime_in_E_prime_v = E_prime_v.get_latest_event_handle().value();

      // When checking whether any event in `dom_[E'](v)` happens before
      // `next_[E'](q)` below for thread `q`, we must consider that the
      // events relative to `E` (this execution) are different than those
      // relative to `E'.v`. Thus e.g. event `7` in `E` may be event `4`
      // in `E'.v`. Since we are asking about "happens-before"
      // `-->_[E'.v]` about `E'.v`, we must build `v` relative to `E'`
      v.push_back(e_prime_in_E_prime_v);

      // Note that we add `q` to v regardless of whether `q` itself has been
      // disqualified since `q` may itself disqualify other actors
      // (i.e. even if `q` is disqualified from being an initial, it
      // is still contained in the sequence `v`)
      const aid_t q = E_prime_v.get_actor_with_handle(e_prime_in_E_prime_v);
      if (disqualified_actors.count(q) > 0) {
        continue;
      }
      const bool is_initial = std::none_of(v.begin(), v.end(), [&](const auto& e_star) {
        return E_prime_v.happens_before(e_star, e_prime_in_E_prime_v);
      });
      if (is_initial) {
        return q;
      } else {
        // If `q` is disqualified as a candidate, clearly
        // no event occurring after `e_prime` in `E` executed
        // by actor `q` will qualify since any (valid) happens-before
        // relation orders actions taken by each actor
        disqualified_actors.insert(q);
      }
    }
  }
  return std::nullopt;
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