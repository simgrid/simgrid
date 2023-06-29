/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_MAXIMAL_SUBSETS_ITERATOR_HPP
#define SIMGRID_MC_UDPOR_MAXIMAL_SUBSETS_ITERATOR_HPP

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/xbt/utils/iter/iterator_wrapping.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <functional>
#include <optional>
#include <stack>
#include <unordered_map>

namespace simgrid::mc::udpor {

/**
 * @brief An iterator over the tree of sets of (non-empty) maximal events that
 * can be generated from a given set of events
 *
 * This iterator traverses all possible sets of maximal events that
 * can be formed from some subset of events of an unfolding,
 * each of which satisfy a predicate.
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
  using node_filter_function       = std::function<bool(const UnfoldingEvent*)>;
  using topological_order_position = std::vector<const UnfoldingEvent*>::const_iterator;

  maximal_subsets_iterator()                                    = default;
  explicit maximal_subsets_iterator(const Configuration& config,
                                    const std::optional<node_filter_function>& filter = std::nullopt,
                                    std::optional<size_t> maximum_subset_size         = std::nullopt)
      : maximal_subsets_iterator(config.get_events(), filter, maximum_subset_size)
  {
  }
  explicit maximal_subsets_iterator(const EventSet& events,
                                    const std::optional<node_filter_function>& filter = std::nullopt,
                                    std::optional<size_t> maximum_subset_size         = std::nullopt);

private:
  std::vector<const UnfoldingEvent*> topological_ordering;

  // The boolean is a bit of an annoyance, but it works. Effectively,
  // there's no way to distinguish between "we're starting the search
  // after the empty set" and "we've finished the search" since the resulting
  // maximal set and backtracking point stack will both be empty in both cases
  bool has_started_searching                              = false;
  std::optional<size_t> maximum_subset_size               = std::nullopt;
  std::optional<EventSet> current_maximal_set             = std::nullopt;
  std::stack<topological_order_position, std::vector<topological_order_position>> backtrack_points;

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
  struct Bookkeeper {
  public:
    using topological_order_position = maximal_subsets_iterator::topological_order_position;

    void mark_included_in_maximal_set(const UnfoldingEvent*);
    void mark_removed_from_maximal_set(const UnfoldingEvent*);
    topological_order_position find_next_candidate_event(topological_order_position first,
                                                         topological_order_position last) const;

  private:
    std::unordered_map<const UnfoldingEvent*, unsigned> event_counts;

    /// @brief Whether or not the given event, according to the
    /// bookkeeping that has been done thus far, can be added to the
    /// current candidate maximal set
    bool is_candidate_event(const UnfoldingEvent*) const;
  };
  Bookkeeper bookkeeper;

  void add_element_to_current_maximal_set(const UnfoldingEvent*);
  void remove_element_from_current_maximal_set(const UnfoldingEvent*);

  /**
   * @brief Moves to the next node in the topological ordering
   * by continuing the search in the tree of maximal event sets
   * from where we currently believe we are in the tree
   *
   * At each stage of the iteration, the iterator points to
   * a maximal event set that can be thought of as `R` + `A`:
   *
   * |   R    | A
   * +--------+
   *
   * where `R` is some set of events and `A` is another event.
   *
   * The iterator first tries expansion from `R` + `A`. If it finds
   * node `B` to expand, this means that there is a node in the tree of
   * maximal event sets of `C` (the configuration traversed) such that
   * `R` + `A` + `B` needs to be checked.
   *
   * If no such node is found, then the iterator must check `R` +
   * some other node AFTER `A`. The new set of possibilities potentially
   * includes some of `A`'s dependencies, so their counts are decremented
   * prior to searching.
   *
   * @note: This method is a mutating method: it manipulates the
   * iterator such that the iterator refers to the next maximal
   * set sans the element returned. The `increment()` function performs
   * the rest of the work needed to actually complete the transition
   *
   * @returns an iterator poiting to the event that should next
   * be added to the set of maximal events if such an event exists,
   * or to the end of the topological ordering if no such event exists
   */
  topological_order_position continue_traversal_of_maximal_events_tree();

  /**
   * @brief: Whether or not the current maximal set can
   * grow based on the size limit imposed on the maximal
   * sets that can be produced
   */
  bool can_grow_maximal_set() const;

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const maximal_subsets_iterator& other) const { return current_maximal_set == other.current_maximal_set; }
  const EventSet& dereference() const
  {
    static const EventSet empty_set;
    if (current_maximal_set.has_value()) {
      return current_maximal_set.value();
    }
    return empty_set;
  }

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

template <typename T>
using maximal_subsets_iterator_wrapper = simgrid::xbt::iterator_wrapping<maximal_subsets_iterator, const T&>;

} // namespace simgrid::mc::udpor
#endif
