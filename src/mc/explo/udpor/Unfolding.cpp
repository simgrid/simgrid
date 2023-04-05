/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Unfolding.hpp"

#include <stdexcept>

namespace simgrid::mc::udpor {

void Unfolding::remove(const EventSet& events)
{
  for (const auto e : events) {
    remove(e);
  }
}

void Unfolding::remove(const UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a non-null pointer to an event, but received NULL");
  }
  this->global_events_.erase(e);
  this->event_handles.remove(e);
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

  if (auto loc = this->find_equivalent(handle); loc != this->end()) {
    // There's already an event in the unfolding that is semantically
    // equivalent. Return the handle to that event and ignore adding in
    // a duplicate event
    return *loc;
  }

  this->event_handles.insert(handle);
  this->global_events_[handle] = std::move(e);
  return handle;
}

EventSet Unfolding::get_immediate_conflicts_of(const UnfoldingEvent* e) const
{
  EventSet immediate_conflicts;
  for (const auto event : *this) {
    if (event->immediately_conflicts_with(e)) {
      immediate_conflicts.insert(event);
    }
  }
  return immediate_conflicts;
}

} // namespace simgrid::mc::udpor
