/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_LAZY_KSUBSETS_HPP
#define XBT_UTILS_LAZY_KSUBSETS_HPP

#include "src/xbt/utils/iter/subsets.hpp"

namespace simgrid::xbt {

template <class Iterable> class LazyKSubsets;
template <class Iterable> LazyKSubsets<Iterable> make_k_subsets_iter(unsigned k, const Iterable& container);

/**
 * @brief A container which "contains" all subsets of
 * size `k` of some other container `WrapperContainer`
 *
 * @note: You should not store instances of this class directly,
 * as it acts more like a simply wrapping around another iterable
 * type to make it more convenient to iterate over subsets of
 * some iterable type.
 *
 * @class Iterable: The container from which
 * the subsets "contained" by this container are derived
 */
template <class Iterable> class LazyKSubsets final {
public:
  auto begin() const
  {
    return subsets_iterator<typename Iterable::const_iterator>(k, iterable.begin(), iterable.end());
  }
  auto end() const { return subsets_iterator<typename Iterable::const_iterator>(k); }

private:
  const unsigned k;
  const Iterable& iterable;
  LazyKSubsets(unsigned k, const Iterable& iterable) : k(k), iterable(iterable) {}

  template <class IterableType>
  friend LazyKSubsets<IterableType> make_k_subsets_iter(unsigned k, const IterableType& iterable);
};

template <class Iterable> LazyKSubsets<Iterable> make_k_subsets_iter(unsigned k, const Iterable& container)
{
  return LazyKSubsets<Iterable>(k, container);
}

} // namespace simgrid::xbt

#endif
