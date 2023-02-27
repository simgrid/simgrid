/* A thread pool (C++ version).                                             */

/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_ITER_SUBSETS_HPP
#define XBT_UTILS_ITER_SUBSETS_HPP

#include <functional>
#include <numeric>
#include <optional>
#include <vector>

namespace simgrid::xbt {

/**
 * @brief A higher-order iterator which traverses all possible subsets
 * of a given fixed size `k` of an iterable sequence
 *
 * @class Iterator: The iterator over which this higher-order iterator
 * generates elements.
 */
template <class Iterator> struct subsets_iterator {
  subsets_iterator();
  subsets_iterator(unsigned k);
  subsets_iterator(unsigned k, Iterator begin, Iterator end = Iterator());

  subsets_iterator& operator++();
  auto operator->() const { return &current_subset; }
  auto& operator*() const { return current_subset; }

  bool operator==(const subsets_iterator<Iterator>& other) const
  {
    if (this->end == std::nullopt and other.end == std::nullopt) {
      return true;
    }
    if (this->k != other.k) {
      return false;
    }
    if (this->k == 0) { // this->k == other.k == 0
      return true;
    }
    return this->end != std::nullopt and other.end != std::nullopt and this->P[0] == other.P[0];
  }
  bool operator!=(const subsets_iterator<Iterator>& other) const { return not(this->operator==(other)); }

  using iterator_category = std::forward_iterator_tag;
  using difference_type   = int; // # of steps between
  using value_type        = std::vector<Iterator>;
  using pointer           = value_type*;
  using reference         = value_type&;

private:
  unsigned k;
  std::optional<Iterator> end = std::nullopt;
  std::vector<Iterator> current_subset;
  std::vector<unsigned> P;
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

template <typename Iterator> subsets_iterator<Iterator>& subsets_iterator<Iterator>::operator++()
{
  if (end == std::nullopt || k == 0) {
    return *this;
  }

  // Move the last element over each time
  ++current_subset[k - 1];
  ++P[k - 1];

  const auto end                  = this->end.value();
  const bool shift_other_elements = current_subset[k - 1] == end;

  if (shift_other_elements) {
    if (k == 1) {
      // We're done in the case that k = 1; here, we've iterated
      // through the list once which is sufficient
      this->end = std::nullopt;
      return *this;
    }

    // At this point, k >= 2

    // The number of elements is now equal to the "index"
    // of the last element (it is at the end, which means we added
    // for the last time)
    const unsigned n = P[k - 1];

    unsigned l = 0;
    for (unsigned j = k - 2; j > 0; j--) {
      if (P[j] != (n - (k - j))) {
        l = j;
        break;
      }
    }

    ++P[l];
    ++current_subset[l];

    // At this point, this means we've sucessfully iterated through
    // all subsets, so we're done
    if (l == 0 and P[0] > (n - k)) {
      this->end = std::nullopt;
      return *this;
    }

    // Otherwise, everyone moves over

    auto iter_at_l = current_subset[l];
    for (auto i = l + 1; i <= (k - 1); i++) {
      P[i]              = P[l] + (i - l);
      current_subset[i] = ++iter_at_l;
    }
  }
  return *this;
}

} // namespace simgrid::xbt

#endif
