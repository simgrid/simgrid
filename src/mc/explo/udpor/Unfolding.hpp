/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_UNFOLDING_HPP
#define SIMGRID_MC_UDPOR_UNFOLDING_HPP

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
  std::unordered_map<UnfoldingEvent*, std::unique_ptr<UnfoldingEvent>> global_events_;

public:
  Unfolding()                       = default;
  Unfolding& operator=(Unfolding&&) = default;
  Unfolding(Unfolding&&)            = default;

  void remove(UnfoldingEvent* e);
  void insert(std::unique_ptr<UnfoldingEvent> e);

  size_t size() const { return this->global_events_.size(); }
  bool empty() const { return this->global_events_.empty(); }
};

} // namespace simgrid::mc::udpor
#endif
