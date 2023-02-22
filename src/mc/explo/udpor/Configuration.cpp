/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

#include <algorithm>
#include <stack>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<UnfoldingEvent*> events) : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(EventSet events) : events_(events)
{
  if (!events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }
}

void Configuration::add_event(UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  if (this->events_.contains(e)) {
    return;
  }

  this->events_.insert(e);
  this->newest_event = e;

  // Preserves the property that the configuration is valid
  History history(e);
  if (!this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

std::vector<UnfoldingEvent*> Configuration::get_topogolically_sorted_events_of_reverse_graph() const
{
  if (events_.empty()) {
    return std::vector<UnfoldingEvent*>();
  }

  std::stack<UnfoldingEvent*> event_stack;
  std::vector<UnfoldingEvent*> topological_ordering;
  EventSet unknown_events = events_, temporarily_marked_events, permanently_marked_events;

  while (not unknown_events.empty()) {
    EventSet discovered_events;
    event_stack.push(*unknown_events.begin());

    while (not event_stack.empty()) {
      UnfoldingEvent* evt = event_stack.top();
      discovered_events.insert(evt);

      if (not temporarily_marked_events.contains(evt)) {
        // If this event hasn't yet been marked, do
        // so now so that if we see it again in a child we can
        // detect a cycle and if we see it again here
        // we can detect that the node is re-processed
        temporarily_marked_events.insert(evt);
      } else {

        // Mark this event as:
        // 1. discovered across all DFSs performed
        // 2. part of the topological search
        temporarily_marked_events.remove(evt);
        permanently_marked_events.insert(evt);
        topological_ordering.push_back(evt);

        // Only now do we remove the event, i.e. once
        // we've processed the same event again
        event_stack.pop();
      }

      const EventSet immediate_causes = evt->get_immediate_causes();
      if (immediate_causes.is_subset_of(temporarily_marked_events)) {
        throw std::invalid_argument("Attempted to perform a topological sort on a configuration "
                                    "whose contents contain a cycle. The configuration (and the graph "
                                    "connecting all of the events) is an invalid event structure");
      }
      const EventSet undiscovered_causes = std::move(immediate_causes).subtracting(discovered_events);

      for (const auto cause : undiscovered_causes) {
        event_stack.push(cause);
      }
    }
  }

  return topological_ordering;
}

} // namespace simgrid::mc::udpor
