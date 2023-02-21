/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

#include <stack>

namespace simgrid::mc::udpor {

History::Iterator::Iterator(const EventSet& initial_events, optional_configuration config)
    : frontier(initial_events), configuration(config)
{
}

History::Iterator& History::Iterator::operator++()
{
  if (not frontier.empty()) {
    // "Pop" the event at the "front"
    UnfoldingEvent* e = *frontier.begin();
    frontier.remove(e);

    // If there is a configuration and if the
    // event is in it, skip it: the event and
    // all of its immediate causes do not need to
    // be searched since the configuration contains
    // them (configuration invariant)
    if (configuration.has_value() && configuration->get().contains(e)) {
      return *this;
    }

    // Mark this event as seen
    current_history.insert(e);

    // Perform the expansion with all viable expansions
    EventSet candidates = e->get_immediate_causes();

    candidates.subtract(current_history);
    frontier.form_union(std::move(candidates));
  }
  return *this;
}

EventSet History::get_all_events() const
{
  auto first      = this->begin();
  const auto last = this->end();

  for (; first != last; ++first)
    ;

  return first.current_history;
}

bool History::contains(UnfoldingEvent* e) const
{
  return std::any_of(this->begin(), this->end(), [=](UnfoldingEvent* e_hist) { return e == e_hist; });
}

EventSet History::get_event_diff_with(const Configuration& config) const
{
  auto wrapped_config = std::optional<std::reference_wrapper<const Configuration>>{config};
  auto first          = Iterator(events_, std::move(wrapped_config));
  const auto last     = this->end();

  for (; first != last; ++first)
    ;

  return first.current_history;
}

} // namespace simgrid::mc::udpor
