/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_MAXIMAL_SUBSETS_ITERATOR_HPP
#define SIMGRID_MC_UDPOR_MAXIMAL_SUBSETS_ITERATOR_HPP

#include "src/mc/explo/udpor/Configuration.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <optional>
#include <stack>
#include <unordered_map>

namespace simgrid::mc::udpor {

/**
 * @brief An iterator over the tree of sets of maximal events that
 * can be generated from a given configuration
 *
 * This iterator traverses all possible sets of maximal events that
 * can be formed from a configuration, each of which satisfy a predicate.
 *
 * Iteration over the maximal events of a configuration is an important
 * step in computing the extension set of a configuration for an action
 * whose identity is not "exploitable" (i.e. one whose type information cannot
 * help us narrow down our search).
 */
struct maximal_subsets_iterator
    : public boost::iterator_facade<maximal_subsets_iterator, const EventSet, boost::forward_traversal_tag> {
public:
  // A function which answers the question "do I need to consider maximal sets
  // that contain this node?"
  using node_filter_function = std::function<bool(const UnfoldingEvent*)>;

  maximal_subsets_iterator();
  maximal_subsets_iterator(const Configuration& config)
      : maximal_subsets_iterator(
            config, [](const UnfoldingEvent*) constexpr { return true; })
  {
  }

  maximal_subsets_iterator(const Configuration& config, node_filter_function filter)
      : config({config})
      , topological_ordering(config.get_topologically_sorted_events_of_reverse_graph())
      , filter(filter)
  {
    // The idea here is that initially, no work has been done; but we want
    // it to be the case that the iterator points at the very first
    // element in the list. Effectively, we want to take the first step
    if (not topological_ordering.empty()) {
      auto earliest_element_iter = topological_ordering.begin();
      // add_element_to_current_maximal_set(*earliest_element_iter);
      backtrack_points.push(earliest_element_iter);
    }
  }

private:
  using topological_order_position = std::vector<UnfoldingEvent*>::const_iterator;
  const std::optional<std::reference_wrapper<const Configuration>> config;
  const std::vector<UnfoldingEvent*> topological_ordering;
  const std::optional<node_filter_function> filter;

  EventSet current_maximal_set = EventSet();
  std::stack<topological_order_position> backtrack_points;

  /**
   * @brief A small class which provides functionality for managing
   * the "counts" as the iterator proceeds forward in time
   *
   * As an instance of the `maximal_subsets_iterator` traverses
   * the configuration, it keeps track of how many events
   * further down in the causality tree have been signaled as in-conflict
   * with events that are its current maximal event set (i.e.
   * its `current_maximal_set`)
   */
  struct bookkeeper {
  private:
    using topological_order_position = maximal_subsets_iterator::topological_order_position;
    std::unordered_map<UnfoldingEvent*, unsigned> event_counts;

    bool is_candidate_event(UnfoldingEvent*) const;

  public:
    void mark_included_in_maximal_set(UnfoldingEvent*);
    void mark_removed_from_maximal_set(UnfoldingEvent*);

    topological_order_position find_next_event(topological_order_position first, topological_order_position last) const;
  } bookkeeper;

  void add_element_to_current_maximal_set(UnfoldingEvent*);
  void remove_element_from_current_maximal_set(UnfoldingEvent*);

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const maximal_subsets_iterator& other) const { return current_maximal_set == other.current_maximal_set; }
  const EventSet& dereference() const { return current_maximal_set; }

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

} // namespace simgrid::mc::udpor
#endif
