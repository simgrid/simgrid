/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_LAZY_POWER_SET_HPP
#define XBT_UTILS_LAZY_POWER_SET_HPP

#include "src/xbt/utils/iter/powerset.hpp"

namespace simgrid::xbt {

template <class Iterable> class LazyPowerset;
template <class Iterable> LazyPowerset<Iterable> make_powerset_iter(const Iterable& container);

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
template <class Iterable> class LazyPowerset final {
public:
  auto begin() const { return powerset_iterator<typename Iterable::const_iterator>(iterable.begin(), iterable.end()); }
  auto end() const { return powerset_iterator<typename Iterable::const_iterator>(); }

private:
  const Iterable& iterable;
  LazyPowerset(const Iterable& iterable) : iterable(iterable) {}
  template <class IterableType> friend LazyPowerset<IterableType> make_powerset_iter(const IterableType& iterable);
};

template <class Iterable> LazyPowerset<Iterable> make_powerset_iter(const Iterable& container)
{
  return LazyPowerset<Iterable>(container);
}

} // namespace simgrid::xbt

#endif
