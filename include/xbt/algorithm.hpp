/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_ALGORITHM_HPP
#define XBT_ALGORITHM_HPP

#include <utility>

namespace simgrid {
namespace xbt {

/** @brief Sorts the elements of the sequence [first, last) according to their color assuming elements can have only
 * three colors.  Since there are only three colors, it is linear and much faster than a classical sort.  See for
 * example http://en.wikipedia.org/wiki/Dutch_national_flag_problem
 *
 * \param first forward iterators to the initial position of the sequence to partition.
 * \param last forward iterators to the final position of the sequence to partition.
 * \param color the color function that accepts an element in the range as argument, and returns a value of 0, 1, or 2.
 *
 * At the end of the call, elements with color 0 are at the beginning of the range, elements with color 2 are at the end
 * and elements with color 1 are in the middle.
 */
template <class ForwardIterator, class ColorFunction>
void three_way_partition(ForwardIterator first, ForwardIterator last, ColorFunction color)
{
  ForwardIterator iter = first;
  while (iter < last) {
    int c = color(*iter);
    if (c == 1) {
      ++iter;
    } else if (c == 0) {
      if (iter != first)
        std::swap(*iter, *first);
      ++iter;
      ++first;
    } else { // c == 2
      --last;
      if (iter != last)
        std::swap(*iter, *last);
    }
  }
}
}
}

#endif
