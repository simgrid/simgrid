/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_COMPATIBILITY_GRAPH_NODE_HPP
#define SIMGRID_MC_UDPOR_COMPATIBILITY_GRAPH_NODE_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <initializer_list>
#include <memory>
#include <unordered_set>

namespace simgrid::mc::udpor {

/**
 * @brief A node in a `CompatabilityGraph` which describes which
 * combinations of events are in causal-conflict with the events
 * associated with the node
 */
class CompatibilityGraphNode {
public:
  CompatibilityGraphNode(const CompatibilityGraphNode&)            = default;
  CompatibilityGraphNode& operator=(CompatibilityGraphNode const&) = default;
  CompatibilityGraphNode(CompatibilityGraphNode&&)                 = default;
  CompatibilityGraphNode(std::unordered_set<CompatibilityGraphNode*> conflicts);

  void add_event(UnfoldingEvent* e);

private:
  EventSet events_;

  /**
   * @brief The nodes with which this node is in conflict with
   */
  std::unordered_set<CompatibilityGraphNode*> conflicts;
};

} // namespace simgrid::mc::udpor
#endif