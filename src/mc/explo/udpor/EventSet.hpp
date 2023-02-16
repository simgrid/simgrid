/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENT_SET_HPP
#define SIMGRID_MC_UDPOR_EVENT_SET_HPP

#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <cstddef>
#include <unordered_set>

namespace simgrid::mc::udpor {

class EventSet {
private:
  std::unordered_set<UnfoldingEvent*> events_;
  explicit EventSet(std::unordered_set<UnfoldingEvent*>&& raw_events) : events_(raw_events) {}

public:
  EventSet()                           = default;
  EventSet(const EventSet&)            = default;
  EventSet& operator=(const EventSet&) = default;
  EventSet& operator=(EventSet&&)      = default;
  EventSet(EventSet&&)                 = default;

  inline auto begin() const { return this->events_.begin(); }
  inline auto end() const { return this->events_.end(); }

  void remove(UnfoldingEvent* e);
  void subtract(const EventSet& other);
  void subtract(const Configuration& other);
  EventSet subtracting(UnfoldingEvent* e) const;
  EventSet subtracting(const EventSet& e) const;
  EventSet subtracting(const Configuration& e) const;

  void insert(UnfoldingEvent* e);
  void form_union(const EventSet&);
  void form_union(const Configuration&);
  EventSet make_union(UnfoldingEvent* e) const;
  EventSet make_union(const EventSet&) const;
  EventSet make_union(const Configuration& e) const;

  size_t size() const;
  bool empty() const;
  bool contains(UnfoldingEvent* e) const;
  bool is_subset_of(const EventSet& other) const;
};

} // namespace simgrid::mc::udpor
#endif
