/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/Resource.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include <simgrid/s4u/Engine.hpp>

namespace simgrid::kernel::profile {

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
  if (heap_.empty())
    s4u::Engine::on_platform_created_cb([this]() {
      /* Handle the events of time = 0 right after the platform creation */
      double next_event_date;
      while ((next_event_date = this->next_date()) != -1.0) {
        if (next_event_date > 0)
          break;

        double value                 = -1.0;
        resource::Resource* resource = nullptr;
        while (auto* event = this->pop_leq(next_event_date, &value, &resource)) {
          if (value >= 0)
            resource->apply_event(event, value);
        }
      }
    });

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
  if (next_date() > date || heap_.empty())
    return nullptr;

  Event* event       = heap_.top().second;
  Profile* profile   = event->profile;
  DatedValue dateVal = profile->next(event);

  *resource = event->resource;
  *value    = dateVal.value_;

  heap_.pop();

  return event;
}
} // namespace simgrid::kernel::profile
