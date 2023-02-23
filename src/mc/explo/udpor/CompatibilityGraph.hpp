/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_COMPATIBILITY_GRAPH_HPP
#define SIMGRID_MC_UDPOR_COMPATIBILITY_GRAPH_HPP

#include "src/mc/explo/udpor/CompatibilityGraphNode.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <memory>
#include <unordered_map>

namespace simgrid::mc::udpor {

/**
 * @brief A graph which describes the causal-conflicts between groups of
 * events from an unfolding
 *
 * A compatibility graph conceptually describes how causal "strands" of an
 * event structure are related to one another. Each node in the graph
 * represents a set of events from which only a single event could be selected
 * if creating a new subset of the events of an unfolding which is *causally-free*.
 * We say a set of events is *causally free* if and only if that set of events is a
 * maximal set of events (i.e., the two conditions are equivalent). We write
 * E(_node_) to denote the set of events associated with that node.
 *
 * The edges of the graph describe which "strands" are in causal conflict with one
 * another. That is, if (u, v) is an edge of compatibility graph G, then at
 * most one event from the set S(u) ∪ S(v) could be selected in a causally-free
 * subset of events of an unfolding.
 *
 * An important property of a compatibility graph G is that the cliques of
 * its complement graph G' (recall that the complement G' of a graph G is the
 * graph which has an edge  (u,v) whenever G lacks an edge (u,v))
 * determine all possible combinations of events which are maximal events;
 * i.e., a clique of size `k` in G' is such that all maximal event sets
 * with events chosen from the union of all events mapped to each node in the
 * clique can be formed. Thus, the problem of determining sets of maximal
 * events of a configuration is equivalent to finding all possible cliques
 * of the compatibility graph. While this is an NP-complete problem in
 * general, it is still a (potentially major) refinement over checking
 * all possible subsets of events of the configuration directly.
 */
class CompatibilityGraph {
private:
  std::unordered_map<CompatibilityGraphNode*, std::unique_ptr<CompatibilityGraphNode>> nodes;

public:
  CompatibilityGraph()                                = default;
  CompatibilityGraph& operator=(CompatibilityGraph&&) = default;
  CompatibilityGraph(CompatibilityGraph&&)            = default;

  void remove(CompatibilityGraphNode* e);
  void insert(std::unique_ptr<CompatibilityGraphNode> e);

  size_t size() const { return this->nodes.size(); }
  bool empty() const { return this->nodes.empty(); }
};

} // namespace simgrid::mc::udpor
#endif