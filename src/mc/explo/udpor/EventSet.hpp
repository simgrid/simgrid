/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENT_SET_HPP
#define SIMGRID_MC_UDPOR_EVENT_SET_HPP

#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <unordered_set>
#include <vector>
#include <xbt/asserts.h>

namespace simgrid::mc::udpor {

class EventSet {
private:
  std::unordered_set<const UnfoldingEvent*> events_;

public:
  EventSet()                           = default;
  EventSet(const EventSet&)            = default;
  EventSet& operator=(const EventSet&) = default;
  EventSet& operator=(EventSet&&)      = default;
  EventSet(EventSet&&)                 = default;
  explicit EventSet(const Configuration& config);
  explicit EventSet(const std::vector<const UnfoldingEvent*>& raw_events)
      : events_(raw_events.begin(), raw_events.end())
  {
  }
  explicit EventSet(std::unordered_set<const UnfoldingEvent*>&& raw_events) : events_(std::move(raw_events)) {}
  explicit EventSet(std::initializer_list<const UnfoldingEvent*> event_list) : events_(std::move(event_list)) {}

  auto begin() const { return this->events_.begin(); }
  auto end() const { return this->events_.end(); }
  auto cbegin() const { return this->events_.cbegin(); }
  auto cend() const { return this->events_.cend(); }

  void remove(const UnfoldingEvent*);
  void subtract(const EventSet&);
  void subtract(const Configuration&);
  EventSet subtracting(const UnfoldingEvent*) const;
  EventSet subtracting(const EventSet&) const;
  EventSet subtracting(const Configuration&) const;

  void insert(const UnfoldingEvent*);
  void form_union(const EventSet&);
  void form_union(const Configuration&);
  EventSet make_union(const UnfoldingEvent*) const;
  EventSet make_union(const EventSet&) const;
  EventSet make_union(const Configuration&) const;
  EventSet make_intersection(const EventSet&) const;
  EventSet get_local_config() const;

  size_t size() const;
  bool empty() const;

  bool contains(const UnfoldingEvent*) const;
  bool contains(const History&) const;
  bool contains_equivalent_to(const UnfoldingEvent*) const;
  bool intersects(const EventSet&) const;
  bool intersects(const History&) const;
  bool is_subset_of(const EventSet&) const;

  bool operator==(const EventSet& other) const { return this->events_ == other.events_; }
  bool operator!=(const EventSet& other) const { return this->events_ != other.events_; }
  std::string to_string() const;

  /**
   * @brief Whether or not this set of events could
   * represent a configuration
   */
  bool is_valid_configuration() const;

  /**
   * @brief Whether or not this set of events is
   * a *maximal event set*, i.e. whether each element
   * of the set causes none of the others
   *
   * A set of events `E` is said to be _maximal_ if
   * it is causally-free. Formally,
   *
   * 1. For each event `e` in `E`, there is no event
   * `e'` in `E` such that `e < e'`
   */
  bool is_maximal() const;

  /**
   * @brief Whether or not this set of events is
   * free of conflicts
   *
   * A set of events `E` is said to be _conflict free_
   * if
   *
   * 1. For each event `e` in `E`, there is no event
   * `e'` in `E` such that `e # e'` where `#` is the
   * conflict relation over the unfolding from
   * which the events `E` are derived
   *
   * @note: This method makes use only of the causality
   * tree of the events in the set; i.e. it determines conflicts
   * based solely on the unfolding and the definition of
   * conflict in an unfolding. Some clever techniques
   * exist for computing conflicts with specialized transition
   * types (only mutexes if I remember correctly) that was
   * referenced in The Anh Pham's thesis. This would require
   * keeping track of information *outside* of any given
   * set and probably doesn't work for all types of transitions
   * anyway.
   */
  bool is_conflict_free() const;

  /**
   * @brief Produces the largest subset of this
   * set of events which is maximal
   */
  EventSet get_largest_maximal_subset() const;

  /**
   * @brief Orders the events of the set such that
   * "more recent" events (i.e. those that are farther down in
   * the event structure's dependency chain) come after those
   * that appeared "farther in the past"
   *
   * @returns a vector `V` with the following property:
   *
   * 1. Let i(e) := C -> I map events to their indices in `V`.
   * For every pair of events e, e' in C, if e < e' then i(e) < i(e')
   *
   * Intuitively, events that are closer to the "bottom" of the event
   * structure appear farther along in the list than those that appear
   * closer to the "top"
   */
  std::vector<const UnfoldingEvent*> get_topological_ordering() const;

  /**
   * @brief Orders the events of set such that
   * "more recent" events (i.e. those that are farther down in
   * the event structure's dependency chain) come before those
   * that appear "farther in the past"
   *
   * @note The events of the event structure are arranged such that
   * e < e' implies a directed edge from e to e'. However, it is
   * also useful to be able to traverse the *reverse* graph (for
   * example when computing the compatibility graph of a configuration),
   * hence the distinction between "reversed" and the method
   * "EventSet::get_topological_ordering()"
   *
   * @returns a vector `V` with the following property:
   *
   * 1. Let i(e) := C -> I map events to their indices in `V`.
   * For every pair of events e, e' in C, if e < e' then i(e) > i(e')
   *
   * Intuitively, events that are closer to the "top" of the event
   * structure appear farther along in the list than those that appear
   * closer to the "bottom"
   */
  std::vector<const UnfoldingEvent*> get_topological_ordering_of_reverse_graph() const;

  /**
   * @brief Moves the event set into a list
   */
  std::vector<const UnfoldingEvent*> move_into_vector() const&&;

  using iterator       = decltype(events_)::iterator;
  using const_iterator = decltype(events_)::const_iterator;
  using value_type     = decltype(events_)::value_type;
};

} // namespace simgrid::mc::udpor
#endif
