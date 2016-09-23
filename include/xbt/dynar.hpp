/* Copyright (c) 2004-2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRId_XBT_DYNAR_HPP
#define SIMGRId_XBT_DYNAR_HPP

#include <boost/range/iterator_range.hpp>
#include <xbt/asserts.h>
#include <xbt/dynar.h>

namespace simgrid {
namespace xbt {

/** A C++ range from a a dynar */
template<class T>
using DynarRange = boost::iterator_range<T*>;

/** Create an iterator range representing a dynar
 *
 *  C++ range loops for `xbt_dynar_t`:
 *
 *  <code>for (auto& x : range<double>(some_dynar)) ++x;</code>
 */
template<class T> inline
DynarRange<T> range(xbt_dynar_t dynar)
{
  xbt_assert(dynar->elmsize == sizeof(T));
  return DynarRange<T>((T*) dynar->data,
    (T*) ((char*) dynar->data + dynar->used * dynar->elmsize));
}

/** Dynar of `T*` which `delete` its values */
template<class T> inline
xbt_dynar_t newDeleteDynar()
{
  return xbt_dynar_new(sizeof(T*),
    [](void* p) { delete *(T**)p; });
}

}
}
#endif
