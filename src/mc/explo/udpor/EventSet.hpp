/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENT_SET_HPP
#define SIMGRID_MC_UDPOR_EVENT_SET_HPP

#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <cstddef>
#include <initializer_list>
#include <unordered_set>

namespace simgrid::mc::udpor {

class EventSet {
private:
  std::unordered_set<UnfoldingEvent*> events_;

public:
  EventSet()                           = default;
  EventSet(const EventSet&)            = default;
  EventSet& operator=(const EventSet&) = default;
  EventSet& operator=(EventSet&&)      = default;
  EventSet(EventSet&&)                 = default;
  explicit EventSet(Configuration&& config);
  explicit EventSet(std::unordered_set<UnfoldingEvent*>&& raw_events) : events_(raw_events) {}
  explicit EventSet(std::initializer_list<UnfoldingEvent*> event_list) : events_(std::move(event_list)) {}

  auto begin() const { return this->events_.begin(); }
  auto end() const { return this->events_.end(); }
  auto cbegin() const { return this->events_.cbegin(); }
  auto cend() const { return this->events_.cend(); }

  void remove(UnfoldingEvent*);
  void subtract(const EventSet&);
  void subtract(const Configuration&);
  EventSet subtracting(UnfoldingEvent*) const;
  EventSet subtracting(const EventSet&) const;
  EventSet subtracting(const Configuration&) const;

  void insert(UnfoldingEvent*);
  void form_union(const EventSet&);
  void form_union(const Configuration&);
  EventSet make_union(UnfoldingEvent*) const;
  EventSet make_union(const EventSet&) const;
  EventSet make_union(const Configuration&) const;

  size_t size() const;
  bool empty() const;
  bool contains(UnfoldingEvent*) const;
  bool contains(const History&) const;
  bool is_subset_of(const EventSet&) const;

  bool operator==(const EventSet& other) const { return this->events_ == other.events_; }
  bool operator!=(const EventSet& other) const { return this->events_ != other.events_; }

public:
  /**
   * @brief Whether or not this set of events could
   * represent a configuration
   */
  bool is_valid_configuration() const;

  /**
   * @brief Whether or not this set of events is
   * a *maximal event set*, i.e. whether each element
   * of the set causes none of the others
   */
  bool is_maximal_event_set() const;
};

} // namespace simgrid::mc::udpor
#endif
