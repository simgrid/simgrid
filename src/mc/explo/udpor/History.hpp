/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_HISTORY_HPP
#define SIMGRID_MC_UDPOR_HISTORY_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

namespace simgrid::mc::udpor {

/**
 * @brief Conceptually describes the local configuration(s) of
 * an event or a collection of events, encoding the history of
 * the events without needing to actually fully compute all of
 * the events contained in the history
 *
 * When you create an instance of `History`, you are effectively
 * making a "lazy" configuration whose elements are the set of
 * causes of a given event (if the `History` constists of a single
 * event) or the union of all causes of all events (if the
 * `History` consists of a set of events).
 *
 * Using a `History` object to represent the history of a set of
 * events is more efficient (and reads more easily) than first
 * computing the entire history of each of the events separately
 * and combining the results. The former can take advantage of the
 * fact that the histories of a set of events overlap greatly, and
 * thus only a single BFS/DFS search over the event structure needs
 * to be performed instead of many isolated searches for each event.
 *
 * The same observation also allows you to compute the difference between
 * a configuration and a history of a set of events. This is used
 * in UDPOR, for example, when determing the difference J / C and
 * when using K-partial alternatives (which computes J as the union
 * of histories of events)
 */
class History {
private:
  /**
   * @brief The events whose history this instance describes
   */
  EventSet events_;

public:
  History()                          = default;
  History(const History&)            = default;
  History& operator=(History const&) = default;
  History(History&&)                 = default;

  explicit History(UnfoldingEvent* e) : events_({e}) {}
  explicit History(EventSet event_set) : events_(std::move(event_set)) {}

  /**
   * @brief Whether or not the given event is contained in the history
   *
   * @note If you only need to determine whether a few events are contained
   * in a history, prefer this method. If, however, you wish to repeatedly
   * determine over time (e.g. over the course of a computation) whether
   * some event is part of the history, it may be better to first compute
   * all events (see `History::get_all_events()`) and reuse this set
   *
   * @param e the event to check
   * @returns whether or not `e` is contained in the collection
   */
  bool contains(UnfoldingEvent* e) const;

  /**
   * @brief Computes all events in the history described by this instance
   *
   * Sometimes, it is useful to compute the entire set of events that
   * comprise the history of some event `e` of some set of events `E`.
   * This method performs that computation.
   *
   * @returns the set of all causal dependencies of all events this
   * history represents. Equivalently, the method returns the full
   * dependency graph for all events in this history
   */
  EventSet get_all_events() const;

  EventSet get_event_diff_with(const EventSet& event_set) const;
  EventSet get_event_diff_with(const Configuration& config) const;
};

} // namespace simgrid::mc::udpor
#endif
