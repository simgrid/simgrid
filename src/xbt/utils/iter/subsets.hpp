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

// template <class Iterable> struct LazyPowerSet {
// };

// template <class Iterable> struct LazyKSubsets {
// private:
//   const Iterable& universe;

// public:
//   LazyKSubsets(const Iterable& universe) : universe(universe) {}
// };

template <class Iterator> struct subsets_iterator {

  subsets_iterator(unsigned k);
  subsets_iterator(unsigned k, Iterator begin, Iterator end = Iterator());

  subsets_iterator& operator++();
  auto operator->() const { return &current_subset; }
  auto& operator*() const { return current_subset; }

  bool operator==(const subsets_iterator<Iterator>& other) const
  {
    if (this->k != other.k) {
      return false;
    }
    if (this->k == 0) {
      return true;
    }
    return this->P[0] == other.P[0];
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
    // The number of elements is now equal to the "index"
    // of the last element (it is at the end, which means we added
    // for the finalth time)
    const unsigned n = P[k - 1];

    auto l = 0;
    for (int j = static_cast<int>(k - 2); j >= 0; j--) {
      if (P[j] != (n - (k - j))) {
        l = j;
        break;
      }
    }

    P[l] += 1;

    if (l == 0 and P[0] > (n - k)) {
      return *this;
    }

    for (auto i = l + 1; i <= (k - 1); i++) {
      P[i] = P[l] + (i - l);
    }
  }

  return *this;
}

} // namespace simgrid::xbt

#endif
