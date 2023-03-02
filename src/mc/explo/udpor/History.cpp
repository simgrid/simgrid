/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

#include <stack>

namespace simgrid::mc::udpor {

History::Iterator::Iterator(const EventSet& initial_events, optional_configuration config)
    : frontier(initial_events), maximal_events(initial_events), configuration(config)
{
  // NOTE: Only events in the initial set of events can ever hope to have
  // a chance at being a maximal event, since all other events in
  // the search are generate by looking at dependencies of these events
  // and all subsequent events that are added by the iterator
}

void History::Iterator::increment()
{
  if (not frontier.empty()) {
    // "Pop" the event at the "front"
    const UnfoldingEvent* e = *frontier.begin();
    frontier.remove(e);

    // If there is a configuration and if the
    // event is in it, skip it: the event and
    // all of its immediate causes do not need to
    // be searched since the configuration contains
    // them (configuration invariant)
    if (configuration.has_value() && configuration->get().contains(e)) {
      return;
    }

    // Mark this event as seen
    current_history.insert(e);

    // Perform the expansion with all viable expansions
    EventSet candidates = e->get_immediate_causes();

    maximal_events.subtract(candidates);

    candidates.subtract(current_history);
    frontier.form_union(candidates);
  }
}

const UnfoldingEvent* const& History::Iterator::dereference() const
{
  return *frontier.begin();
}

bool History::Iterator::equal(const Iterator& other) const
{
  // If what the iterator sees next is the same, we consider them
  // to be the same iterator. This way, once the iterator has completed
  // its search, it will be "equal" to an iterator searching nothing
  return this->frontier == other.frontier;
}

EventSet History::get_all_events() const
{
  auto first      = this->begin();
  const auto last = this->end();

  for (; first != last; ++first)
    ;

  return first.current_history;
}

EventSet History::get_all_maximal_events() const
{
  auto first      = this->begin();
  const auto last = this->end();

  for (; first != last; ++first)
    ;

  return first.maximal_events;
}

bool History::contains(const UnfoldingEvent* e) const
{
  return std::any_of(this->begin(), this->end(), [=](const UnfoldingEvent* e_hist) { return e == e_hist; });
}

EventSet History::get_event_diff_with(const Configuration& config) const
{
  auto wrapped_config = std::optional<std::reference_wrapper<const Configuration>>{config};
  auto first          = Iterator(events_, wrapped_config);
  const auto last     = this->end();

  for (; first != last; ++first)
    ;

  return first.current_history;
}

} // namespace simgrid::mc::udpor
