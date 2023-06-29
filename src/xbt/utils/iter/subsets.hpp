/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_ITER_SUBSETS_HPP
#define XBT_UTILS_ITER_SUBSETS_HPP

#include <boost/iterator/iterator_facade.hpp>
#include <functional>
#include <numeric>
#include <optional>
#include <vector>

namespace simgrid::xbt {

/**
 * @brief A higher-order forward-iterator which traverses all possible subsets
 * of a given fixed size `k` of an iterable sequence
 *
 * @class Iterator: The iterator over which this higher-order iterator
 * generates elements.
 */
template <class Iterator>
struct subsets_iterator : public boost::iterator_facade<subsets_iterator<Iterator>, const std::vector<Iterator>,
                                                        boost::forward_traversal_tag> {
  subsets_iterator();
  explicit subsets_iterator(unsigned k);
  explicit subsets_iterator(unsigned k, Iterator begin, Iterator end = Iterator());

private:
  unsigned k; // The size of the subsets generated
  std::optional<Iterator> end = std::nullopt;
  std::vector<Iterator> current_subset;
  std::vector<unsigned> P; // Increment counts to determine which combinations need to be traversed

  // boost::iterator_facade<...> interface to implement
  void increment();
  bool equal(const subsets_iterator<Iterator>& other) const;
  const std::vector<Iterator>& dereference() const;

  // Allows boost::iterator_facade<...> to function properly
  friend class boost::iterator_core_access;
};

template <typename Iterator> subsets_iterator<Iterator>::subsets_iterator() : subsets_iterator<Iterator>(0) {}

template <typename Iterator>
subsets_iterator<Iterator>::subsets_iterator(unsigned k)
    : k(k), current_subset(std::vector<Iterator>(k)), P(std::vector<unsigned>(k))
{
  std::iota(P.begin(), P.end(), k);
}

template <typename Iterator>
subsets_iterator<Iterator>::subsets_iterator(unsigned k, Iterator begin, Iterator end)
    : k(k), end(std::optional<Iterator>{end}), current_subset(std::vector<Iterator>(k)), P(std::vector<unsigned>(k))
{
  for (unsigned i = 0; i < k; i++) {
    // Less than `k` elements to choose
    if (begin == end) {
      // We want to initialize the object then to be equivalent
      // to the end iterator so that there are no items to iterate
      // over
      this->end = std::nullopt;
      std::iota(P.begin(), P.end(), k);
      return;
    }
    current_subset[i] = begin++;
  }
  std::iota(P.begin(), P.end(), 0);
}

template <typename Iterator> bool subsets_iterator<Iterator>::equal(const subsets_iterator<Iterator>& other) const
{
  if (this->end == std::nullopt && other.end == std::nullopt) {
    return true;
  }
  if (this->k != other.k) {
    return false;
  }
  if (this->k == 0) { // this->k == other.k == 0
    return true;
  }
  return this->end != std::nullopt && other.end != std::nullopt && this->P[0] == other.P[0];
}

template <typename Iterator> const std::vector<Iterator>& subsets_iterator<Iterator>::dereference() const
{
  return this->current_subset;
}

template <typename Iterator> void subsets_iterator<Iterator>::increment()
{
  // If k == 0, there's nothing to do
  // If end == std::nullopt, we've finished
  // iterating over all subsets of size `k`
  if (end == std::nullopt || k == 0) {
    return;
  }

  // Move the last element over each time
  ++current_subset[k - 1];
  ++P[k - 1];

  const bool shift_other_elements = current_subset[k - 1] == end.value();

  if (shift_other_elements) {
    if (k == 1) {
      // We're done in the case that k = 1; here, we've iterated
      // through the list once, which is all that is needed
      end = std::nullopt;
      return;
    }

    // At this point, k >= 2

    // The number of elements is now equal to the "index"
    // of the last element (it is at the end, which means we added
    // for the last time)
    const unsigned n = P[k - 1];

    // We're looking to determine
    //
    // argmax_{0 <= j <= k - 2}(P[j] != (n - (k - j)))
    //
    // If P[j] == (n - (k - j)) for some `j`, that means
    // the `j`th element of the current subset has moved
    // "as far as it can move" to the right; in other words,
    // this is our signal that some element before the `j`th
    // has to move over
    //
    // std::max_element() would work here too, but it seems
    // overkill to create a vector full of numbers when a simple
    // range-based for-loop can do the trick
    unsigned l = 0;
    for (unsigned j = k - 2; j > 0; j--) {
      if (P[j] != (n - (k - j))) {
        l = j;
        break;
      }
    }

    ++P[l];
    ++current_subset[l];

    // Plugging in `j = 0` to the above formula yields
    // `n - k`, which is the furthest point that the first (i.e. `0th`)
    // element can be located. Thus, if `P[0] > (n - k)`, this means
    // we've sucessfully iterated through all subsets so we're done
    if (P[0] > (n - k)) {
      end = std::nullopt;
      return;
    }

    // Otherwise, all elements past element `l` are reset
    // to follow one after another immediately after `l`
    auto iter_at_l = current_subset[l];
    for (auto i = l + 1; i <= (k - 1); i++) {
      P[i]              = P[l] + (i - l);
      current_subset[i] = ++iter_at_l;
    }
  }
}

} // namespace simgrid::xbt

#endif
