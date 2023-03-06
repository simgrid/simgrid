/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_LAZY_POWER_SET_HPP
#define XBT_UTILS_LAZY_POWER_SET_HPP

#include "src/xbt/utils/iter/iterator_wrapping.hpp"
#include "src/xbt/utils/iter/powerset.hpp"

namespace simgrid::xbt {

template <class Iterable, class... Args>
using LazyPowerset = iterator_wrapping<powerset_iterator<typename Iterable::const_iterator>, Args...>;

template <class Iterable> constexpr auto make_powerset_iter(const Iterable& container)
{
  return make_iterator_wrapping<powerset_iterator<typename Iterable::const_iterator>>(container.begin(),
                                                                                      container.end());
}

} // namespace simgrid::xbt

#endif
