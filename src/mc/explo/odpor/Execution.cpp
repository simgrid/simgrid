/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/Execution.hpp"
#include <exception>
#include <limits>

namespace simgrid::mc::odpor {

std::unordered_set<Execution::EventHandle> Execution::get_racing_events_of(Execution::EventHandle target) const
{
  std::unordered_set<Execution::EventHandle> racing_events;
  std::unordered_set<Execution::EventHandle> disqualified_events;

  // For each event of the execution
  for (auto e_i = target; e_i > 0 && e_i != std::numeric_limits<Execution::EventHandle>::max(); e_i--) {
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

bool Execution::happens_before(Execution::EventHandle e1_handle, Execution::EventHandle e2_handle) const
{
  // 1. "happens-before" is a subset of "occurs before"
  if (e1_handle > e2_handle) {
    return false;
  }

  // Each execution maintains a stack of clock vectors which are updated
  // according to the procedure outlined in section 4 of the original DPOR paper
  const Event& e2     = get_event_with_handle(e2_handle);
  const aid_t proc_e1 = get_actor_with_handle(e1_handle);
  return e1_handle <= e2.get_clock_vector().get(proc_e1).value_or(0);
}

Execution Execution::get_prefix_up_to(Execution::EventHandle handle)
{
  if (handle == static_cast<Execution::EventHandle>(0)) {
    return Execution();
  }
  return Execution(std::vector<Event>{contents_.begin(), contents_.begin() + (handle - 1)});
}

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
  // The entry in the vector for `t->aid_` is the size
  // of the new stack, which will have a size one greater
  // than that before we insert the new events
  max_clock_vector[t->aid_] = this->size() + 1;
  contents_.push_back(Event({t, max_clock_vector}));
}

void Execution::pop_latest()
{
  contents_.pop_back();
}

} // namespace simgrid::mc::odpor