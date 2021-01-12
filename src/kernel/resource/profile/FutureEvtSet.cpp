/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/Profile.hpp"

namespace simgrid {
namespace kernel {
namespace profile {

simgrid::kernel::profile::FutureEvtSet future_evt_set; // FIXME: singleton antipattern

FutureEvtSet::FutureEvtSet() = default;
FutureEvtSet::~FutureEvtSet()
{
  while (not heap_.empty()) {
    delete heap_.top().second;
    heap_.pop();
  }
}

/** @brief Schedules an event to a future date */
void FutureEvtSet::add_event(double date, Event* evt)
{
  heap_.emplace(date, evt);
}

/** @brief returns the date of the next occurring event (or -1 if empty) */
double FutureEvtSet::next_date() const
{
  return heap_.empty() ? -1.0 : heap_.top().first;
}

/** @brief Retrieves the next occurring event, or nullptr if none happens before date */
Event* FutureEvtSet::pop_leq(double date, double* value, resource::Resource** resource)
{
  double event_date = next_date();
  if (event_date > date || heap_.empty())
    return nullptr;

  Event* event       = heap_.top().second;
  Profile* profile   = event->profile;
  DatedValue dateVal = profile->next(event);

  *resource = event->resource;
  *value    = dateVal.value_;

  heap_.pop();

  return event;
}
} // namespace profile
} // namespace kernel
} // namespace simgrid
