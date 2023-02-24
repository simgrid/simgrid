/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CONFIGURATION_HPP
#define SIMGRID_MC_UDPOR_CONFIGURATION_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <initializer_list>

namespace simgrid::mc::udpor {

class Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&)                 = default;

  Configuration(EventSet events);
  Configuration(std::initializer_list<UnfoldingEvent*> events);

  auto begin() const { return this->events_.begin(); }
  auto end() const { return this->events_.end(); }

  bool contains(UnfoldingEvent* e) const { return this->events_.contains(e); }
  const EventSet& get_events() const { return this->events_; }
  UnfoldingEvent* get_latest_event() const { return this->newest_event; }

  /**
   * @brief Insert a new event into the configuration
   *
   * When the newly added event is inserted into the configuration,
   * an assertion is made to ensure that the configuration remains
   * valid, i.e. that the newly added event's dependencies are contained
   * within the configuration.
   *
   * @param e the event to add to the configuration. If the event is
   * already a part of the configuration, calling this method has no
   * effect.
   *
   * @throws an invalid argument exception is raised should the event
   * be missing one of its dependencies
   *
   * @note: UDPOR technically enforces the invariant that all newly-added events
   * will ensure that the configuration is valid. We perform extra checks to ensure
   * that UDPOR is implemented as expected. There is a performance penalty that
   * should be noted: checking for maximality requires ensuring that all events in the
   * configuration have their dependencies containes within the configuration, which
   * essentially means performing a BFS/DFS over the configuration using a History object.
   * However, since the slowest part of UDPOR involves enumerating all
   * subsets of maximal events and computing k-partial alternatives (the
   * latter definitively an NP-hard problem when optimal), Amdahl's law suggests
   * we shouldn't focus so much on this (let alone the additional benefit of the
   * assertions)
   */
  void add_event(UnfoldingEvent* e);

private:
  /**
   * @brief The most recent event added to the configuration
   */
  UnfoldingEvent* newest_event = nullptr;

  /**
   * @brief The events which make up this configuration
   *
   * @invariant For each event `e` in `events_`, the set of
   * dependencies of `e` is also contained in `events_`
   */
  EventSet events_;
};

} // namespace simgrid::mc::udpor
#endif
