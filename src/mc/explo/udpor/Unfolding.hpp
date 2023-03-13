/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_UNFOLDING_HPP
#define SIMGRID_MC_UDPOR_UNFOLDING_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <memory>
#include <unordered_map>

namespace simgrid::mc::udpor {

class Unfolding {
private:
  /**
   * @brief All of the events that are currently are a part of the unfolding
   *
   * @invariant Each unfolding event maps itself to the owner of that event,
   * i.e. the unique pointer that owns the address. The Unfolding owns all
   * of the addresses that are referenced by EventSet instances and Configuration
   * instances. UDPOR guarantees that events are persisted for as long as necessary
   */
  std::unordered_map<const UnfoldingEvent*, std::unique_ptr<UnfoldingEvent>> global_events_;

  /**
   * @brief: The collection of events in the unfolding
   *
   * @invariant: All of the events in this set are elements of `global_events_`
   * and is kept updated at the same time as `global_events_`
   */
  EventSet event_handles;

public:
  Unfolding()                       = default;
  Unfolding& operator=(Unfolding&&) = default;
  Unfolding(Unfolding&&)            = default;

  void remove(const UnfoldingEvent* e);
  void remove(const EventSet& events);
  void insert(std::unique_ptr<UnfoldingEvent> e);
  bool contains_event_equivalent_to(const UnfoldingEvent* e) const;

  auto begin() const { return this->event_handles.begin(); }
  auto end() const { return this->event_handles.begin(); }
  auto cbegin() const { return this->event_handles.cbegin(); }
  auto cend() const { return this->event_handles.cend(); }
  size_t size() const { return this->global_events_.size(); }
  bool empty() const { return this->global_events_.empty(); }

  /// @brief Computes "#ⁱ_U(e)" for the given event
  EventSet get_immediate_conflicts_of(const UnfoldingEvent*) const;
};

} // namespace simgrid::mc::udpor
#endif
