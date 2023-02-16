/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CONFIGURATION_HPP
#define SIMGRID_MC_UDPOR_CONFIGURATION_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

namespace simgrid::mc::udpor {

class Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&)                 = default;

  auto begin() const { return this->events_.begin(); }
  auto end() const { return this->events_.end(); }
  const EventSet& get_events() const { return this->events_; }
  const EventSet& get_maximal_events() const { return this->max_events_; }
  UnfoldingEvent* get_latest_event() const { return this->newest_event; }

  void add_event(UnfoldingEvent*);

private:
  /**
   * @brief The most recent event added to the configuration
   */
  UnfoldingEvent* newest_event = nullptr;

  /**
   * @brief The events which make up this configuration
   */
  EventSet events_;

  /**
   * The <-maxmimal events of the configuration. These are
   * dynamically adjusted as events are added to the configuration
   *
   * @invariant: Each event that is part of this set is
   *
   * 1. a <-maxmimal event of the configuration, in the sense that
   * there is no event in the configuration that is "greater" than it.
   * In UDPOR terminology, each event does not "cause" another event
   *
   * 2. contained in the set of events `events_` which comprise
   * the configuration
   */
  EventSet max_events_;

private:
  void recompute_maxmimal_events(UnfoldingEvent* new_event);
};

} // namespace simgrid::mc::udpor
#endif
