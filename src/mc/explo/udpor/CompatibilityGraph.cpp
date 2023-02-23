/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/CompatibilityGraph.hpp"

#include <stdexcept>

namespace simgrid::mc::udpor {

// TODO: Remove duplication between the CompatibilityGraph
// and the Unfolding: they are practically identical

void CompatibilityGraph::remove(CompatibilityGraphNode* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a non-null pointer to an event, but received NULL");
  }
  this->nodes.erase(e);
}

void CompatibilityGraph::insert(std::unique_ptr<CompatibilityGraphNode> e)
{
  CompatibilityGraphNode* handle = e.get();
  auto loc                       = this->nodes.find(handle);
  if (loc != this->nodes.end()) {
    // This is bad: someone wrapped the raw event address twice
    // in two different unique ptrs and attempted to
    // insert it into the unfolding...
    throw std::invalid_argument("Attempted to insert a node owned twice."
                                "This will result in a  double free error and must be fixed.");
  }

  // Map the handle to its owner
  this->nodes[handle] = std::move(e);
}

} // namespace simgrid::mc::udpor
