/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"

#include <stdexcept>

namespace simgrid::mc::udpor {

long Unfolding::expanded_events_ = 0;

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

  EventSet immediate_conflicts_of_e = this->compute_immediate_conflicts_of(handle);
  for (auto const eprime : immediate_conflicts_of_e) {
    if (this->immediate_conflicts_.count(eprime) > 0)

      this->immediate_conflicts_.at(eprime).insert(handle);
    else
      this->immediate_conflicts_.insert({eprime, EventSet({handle})});
  }
  immediate_conflicts_.insert({handle, immediate_conflicts_of_e});
  if (Exploration::get_instance() != nullptr) {
    Exploration::get_instance()->dot_output("%u [tooltip=\"Actor %ld : %s\"]\n", handle->get_id(), handle->get_actor(),
                                            handle->get_transition()->to_string(true).c_str());
    Exploration::get_instance()->dot_output("%s", handle->to_dot_string().c_str());
  }
  expanded_events_++;
  return handle;
}

EventSet Unfolding::compute_immediate_conflicts_of(const UnfoldingEvent* e) const
{
  EventSet immediate_conflicts;
  EventSet skippable{};
  for (const auto* event : U) {
    if (skippable.contains(event))
      continue;
    if (event->immediately_conflicts_with(e)) {
      immediate_conflicts.insert(event);
      skippable.form_union(event->get_history());
    }
  }
  return immediate_conflicts;
}

EventSet Unfolding::get_immediate_conflicts_of(const UnfoldingEvent* e) const
{
  if (immediate_conflicts_.count(e) > 0)
    return immediate_conflicts_.at(e);
  else
    return EventSet();
}

} // namespace simgrid::mc::udpor
