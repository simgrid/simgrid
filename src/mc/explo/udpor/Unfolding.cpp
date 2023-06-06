/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Unfolding.hpp"

#include <stdexcept>

namespace simgrid::mc::udpor {

void Unfolding::mark_finished(const EventSet& events)
{
  for (const auto* e : events) {
    mark_finished(e);
  }
}

void Unfolding::mark_finished(const UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a non-null pointer to an event, but received NULL");
  }
  this->U.remove(e);
  this->G.insert(e);
}

const UnfoldingEvent* Unfolding::insert(std::unique_ptr<UnfoldingEvent> e)
{
  const UnfoldingEvent* handle = e.get();
  if (auto loc = this->global_events_.find(handle); loc != this->global_events_.end()) {
    // This is bad: someone wrapped the raw event address twice
    // in two different unique ptrs and attempted to
    // insert it into the unfolding...
    throw std::invalid_argument("Attempted to insert an unfolding event owned twice."
                                "This will result in a  double free error and must be fixed.");
  }

  // Attempt to search first for an event in `U`. If it exists, we use that event
  // instead of `e` since it is semantically equivalent to `e` (i.e. `e` is
  // effectively already contained in the unfolding)
  if (auto loc = std::find_if(U.begin(), U.end(), [=](const auto e_i) { return *e_i == *handle; }); loc != U.end()) {
    // Return the handle to that event and ignore adding in a duplicate event
    return *loc;
  }

  // Then look for `e` in `G`. It's possible `e` was already constructed
  // in the past, in which case we can simply re-use it.
  //
  // Note, though, that in this case we must move the event in `G` into
  // `U`: we've inserted `e` into the unfolding, so we expect it to be in `U`
  if (auto loc = std::find_if(G.begin(), G.end(), [=](const auto e_i) { return *e_i == *handle; }); loc != G.end()) {
    const auto* e_equiv = *loc;
    G.remove(e_equiv);
    U.insert(e_equiv);
    return e_equiv;
  }

  // Otherwise `e` is truly a "new" event
  this->U.insert(handle);
  this->event_handles.insert(handle);
  this->global_events_[handle] = std::move(e);
  return handle;
}

EventSet Unfolding::get_immediate_conflicts_of(const UnfoldingEvent* e) const
{
  EventSet immediate_conflicts;
  for (const auto* event : U) {
    if (event->immediately_conflicts_with(e)) {
      immediate_conflicts.insert(event);
    }
  }
  return immediate_conflicts;
}

} // namespace simgrid::mc::udpor
