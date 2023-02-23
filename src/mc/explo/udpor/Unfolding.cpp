/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Unfolding.hpp"

#include <stdexcept>

namespace simgrid::mc::udpor {

void Unfolding::remove(UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a non-null pointer to an event, but received NULL");
  }
  this->global_events_.erase(e);
}

void Unfolding::insert(std::unique_ptr<UnfoldingEvent> e)
{
  UnfoldingEvent* handle = e.get();
  auto loc               = this->global_events_.find(handle);
  if (loc != this->global_events_.end()) {
    // This is bad: someone wrapped the raw event address twice
    // in two different unique ptrs and attempted to
    // insert it into the unfolding...
    throw std::invalid_argument("Attempted to insert an unfolding event owned twice."
                                "This will result in a  double free error and must be fixed.");
  }

  // Map the handle to its owner
  this->global_events_[handle] = std::move(e);
}

} // namespace simgrid::mc::udpor
