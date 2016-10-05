/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_RANGE_HPP
#define SIMGRID_XBT_RANGE_HPP

namespace simgrid {
namespace xbt {

/** Describes a contiguous inclusive-exclusive [a,b) range of values */
template<class T> class Range {
  T begin_;
  T end_;
public:
  Range()               : begin_(), end_() {}
  Range(T begin, T end) : begin_(std::move(begin)), end_(std::move(end)) {}
  Range(T value) : begin_(value), end_(value + 1) {}
  T& begin()             { return begin_; }
  T& end()               { return end_; }
  const T& begin() const { return begin_; }
  const T& end() const   { return end_; }
  bool empty() const     { return begin_ >= end_; }
  bool contain(T const& x) const { return begin_ <= x && end_ > x; }
};

}
}

#endif
