/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_ITER_VARIABLE_FOR_LOOP_HPP
#define XBT_UTILS_ITER_VARIABLE_FOR_LOOP_HPP

#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>

namespace simgrid::xbt {

/**
 * @brief A higher-order forward-iterator which traverses all possible
 * combinations of selections of elements from a collection of iterable
 * sequences
 *
 * This iterator provides a means of iteratively traversing all combinations
 * of elements of `k` collections (albeit of a single type), selecting a
 * single element from each of the `k` collections in the same way a
 * nested for-loop may select a set of elements. The benefit is that
 * you do not need to actually physically write the for-loop statements
 * directly, and you can dynamically adjust the number of levels of the
 * for-loop according to the situation
 *
 * @class IterableType: The collections from which this iterator
 * selects elements
 */
template <class IterableType>
struct variable_for_loop : public boost::iterator_facade<variable_for_loop<IterableType>,
                                                         const std::vector<typename IterableType::const_iterator>,
                                                         boost::forward_traversal_tag> {
public:
  using underlying_iterator = typename IterableType::const_iterator;

  variable_for_loop() = default;
  explicit variable_for_loop(std::initializer_list<std::reference_wrapper<IterableType>> initializer_list)
      : variable_for_loop(std::vector<std::reference_wrapper<IterableType>>(initializer_list))
  {
  }
  explicit variable_for_loop(std::vector<std::reference_wrapper<IterableType>> collections)
  {
    // All collections should be non-empty: if one is empty, the
    // for-loop has no effect (since there would be no way to choose
    // one element from the empty collection(s))
    const auto has_effect =
        std::none_of(collections.begin(), collections.end(), [](const auto& c) { return c.get().empty(); });

    if (has_effect && (not collections.empty())) {
      std::transform(collections.begin(), collections.end(), std::back_inserter(current_subset),
                     [](const auto& c) { return c.get().cbegin(); });
      underlying_collections = std::move(collections);
    }
    // Otherwise leave `underlying_collections` as default-initialized (i.e. empty)
  }

private:
  std::vector<std::reference_wrapper<IterableType>> underlying_collections;
  std::vector<underlying_iterator> current_subset;

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const variable_for_loop<IterableType>& other) const { return current_subset == other.current_subset; }
  const std::vector<underlying_iterator>& dereference() const { return current_subset; }

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

template <typename IterableType> void variable_for_loop<IterableType>::increment()
{
  // Termination occurs when `current_subset := the empty set`
  // or if we have nothing to iterate over
  if (current_subset.empty() || underlying_collections.empty()) {
    return;
  }

  bool completed_iteration = true;
  const size_t k           = underlying_collections.size() - 1;

  for (auto j = k; j != std::numeric_limits<size_t>::max(); j--) {
    // Attempt to move to the next element of the `j`th collection
    ++current_subset[j];

    // If the `j`th element has reached its own end, reset it
    // back to the beginning and keep moving forward
    if (current_subset[j] == underlying_collections[j].get().cend()) {
      current_subset[j] = underlying_collections[j].get().cbegin();
    } else {
      // Otherwise we've found the largest element which needed to
      // be moved down. Everyone else before us has been reset
      // to properly to point at their beginnings
      completed_iteration = false;
      break;
    }
  }

  if (completed_iteration) {
    // We've iterated over all subsets at this point:
    // set the termination condition
    current_subset.clear();
    return;
  }
}

} // namespace simgrid::xbt

#endif
