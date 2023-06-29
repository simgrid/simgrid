/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_ITER_POWERSET_HPP
#define XBT_UTILS_ITER_POWERSET_HPP

#include "src/xbt/utils/iter/subsets.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <optional>

namespace simgrid::xbt {

/**
 * @brief A higher-order iterator which traverses all possible subsets
 * of the elements of an iterable sequence.
 *
 * @note The power set `P(S)` of any finite set `S` is of exponential size relative
 * to the size of `S`. Be warned that attempting to iterate over the power set of
 * large sets will cost a LOT of time (but that the memory footprint will remain
 * very small). Alas, unless P = NP, there sometimes isn't a better known way to
 * solve a problem (e.g. determining all cliques in a graph)
 *
 * @class Iterator: The iterator over which this higher-order iterator
 * generates elements
 */
template <class Iterator>
struct powerset_iterator : public boost::iterator_facade<powerset_iterator<Iterator>, const std::vector<Iterator>,
                                                         boost::forward_traversal_tag> {
  powerset_iterator()                                                                 = default;
  explicit powerset_iterator(Iterator begin, Iterator end = Iterator());

private:
  // The current size of the subsets
  unsigned n = 0;
  std::optional<Iterator> iterator_begin;
  std::optional<Iterator> iterator_end;
  std::optional<subsets_iterator<Iterator>> current_subset_iter     = std::nullopt;
  std::optional<subsets_iterator<Iterator>> current_subset_iter_end = std::nullopt;

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const powerset_iterator<Iterator>& other) const;
  const std::vector<Iterator>& dereference() const;

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

template <typename Iterator>
powerset_iterator<Iterator>::powerset_iterator(Iterator begin, Iterator end)
    : iterator_begin({begin})
    , iterator_end({end})
    , current_subset_iter({subsets_iterator<Iterator>(0, begin, end)})
    , current_subset_iter_end({subsets_iterator<Iterator>(0)})
{
}

template <typename Iterator> bool powerset_iterator<Iterator>::equal(const powerset_iterator<Iterator>& other) const
{
  return current_subset_iter == other.current_subset_iter;
}

template <typename Iterator> const std::vector<Iterator>& powerset_iterator<Iterator>::dereference() const
{
  if (current_subset_iter.has_value()) {
    return *current_subset_iter.value();
  }
  static const std::vector<Iterator> empty_subset;
  return empty_subset;
}

template <typename Iterator> void powerset_iterator<Iterator>::increment()
{
  if (not current_subset_iter.has_value() || not current_subset_iter_end.has_value() ||
      not current_subset_iter.has_value() || not iterator_end.has_value()) {
    return; // We've traversed all subsets at this point, or we're the "last" iterator
  }

  // Move the underlying subset iterator over
  ++current_subset_iter.value();

  // If the current underlying subset iterator for size `n`
  // is finished, generate one for `n = n + 1`
  if (current_subset_iter == current_subset_iter_end) {
    n++;
    current_subset_iter     = {subsets_iterator<Iterator>(n, iterator_begin.value(), iterator_end.value())};
    current_subset_iter_end = {subsets_iterator<Iterator>(n)};

    // If we've immediately hit a deadend, then we know that the last
    // value of `n` was the number of elements in the iteration, so
    // we're done
    if (current_subset_iter == current_subset_iter_end) {
      current_subset_iter     = std::nullopt;
      current_subset_iter_end = std::nullopt;
    }
  }
}

} // namespace simgrid::xbt

#endif
