/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "xbt/asserts.h"

#include <algorithm>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<const UnfoldingEvent*> events)
    : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(const UnfoldingEvent* e) : Configuration(e->get_history())
{
  // The local configuration should always be a valid configuration. We
  // check the invariant regardless as a sanity check
}

Configuration::Configuration(const EventSet& events) : events_(events)
{
  if (!events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }
}

void Configuration::add_event(const UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  if (this->events_.contains(e)) {
    return;
  }

  // Preserves the property that the configuration is conflict-free
  if (e->conflicts_with(*this)) {
    throw std::invalid_argument("The newly added event conflicts with the events already "
                                "contained in the configuration. Adding this event violates "
                                "the property that a configuration is conflict-free");
  }

  this->events_.insert(e);
  this->newest_event = e;

  // Preserves the property that the configuration is causally closed
  if (auto history = History(e); !this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

bool Configuration::is_compatible_with(const History& history) const
{
  return false;
}

std::vector<const UnfoldingEvent*> Configuration::get_topologically_sorted_events() const
{
  return this->events_.get_topological_ordering();
}

std::vector<const UnfoldingEvent*> Configuration::get_topologically_sorted_events_of_reverse_graph() const
{
  return this->events_.get_topological_ordering_of_reverse_graph();
}

EventSet Configuration::get_minimally_reproducible_events() const
{
  // The implementation exploits the following observations:
  //
  // To select the smallest reproducible set of events, we want
  // to pick events that "knock out" a lot of others. Furthermore,
  // we need to ensure that the events furthest down in the
  // causality graph are also selected. If you combine these ideas,
  // you're basically left with traversing the set of maximal
  // subsets of C! And we have an iterator for that already!
  //
  // The next observation is that the moment we don't increase in size
  // the current maximal set (or decrease the number of events),
  // we know that the prior set `S` covered the entire history of C and
  // was maximal. Subsequent sets will miss events earlier in the
  // topological ordering that appear in `S`
  EventSet minimally_reproducible_events = EventSet();

  for (const auto& maximal_set : maximal_subsets_iterator_wrapper<Configuration>(*this)) {
    if (maximal_set.size() > minimally_reproducible_events.size()) {
      minimally_reproducible_events = maximal_set;
    } else {
      // The moment we see the iterator generate a set of size
      // that is not monotonically increasing, we can stop:
      // the set prior was the minimally-reproducible one
      return minimally_reproducible_events;
    }
  }
  return minimally_reproducible_events;
}

} // namespace simgrid::mc::udpor
