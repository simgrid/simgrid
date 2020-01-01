/* Copyright (c) 2016-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILITY_HPP
#define XBT_UTILITY_HPP

#include <tuple>
#include <functional>

namespace simgrid {
namespace xbt {

/** @brief A hash which works with more stuff
 *
 *  It can hash pairs: the standard hash currently doesn't include this.
 */
template <class X> class hash : public std::hash<X> {
};

template <class X, class Y> class hash<std::pair<X, Y>> {
public:
  std::size_t operator()(std::pair<X, Y> const& x) const
  {
    hash<X> h1;
    hash<X> h2;
    return h1(x.first) ^ h2(x.second);
  }
};

/** @brief Comparator class for using with std::priority_queue or boost::heap.
 *
 * Compare two std::pair by their first element (of type double), and return true when the first is greater than the
 * second.  Useful to have priority queues with the smallest element on top.
 */
template <class Pair> class HeapComparator {
public:
  bool operator()(const Pair& a, const Pair& b) const { return a.first > b.first; }
};

/** @brief Erase an element given by reference from a boost::intrusive::list.
 */
template <class List, class Elem> inline void intrusive_erase(List& list, Elem& elem)
{
  list.erase(list.iterator_to(elem));
}

// integer_sequence and friends from C++14
// We need them to implement `apply` from C++17.

/** A compile-time sequence of integers (from C++14)
 *
 * `index_sequence<std::size_t,1,5,7,9>` represents the sequence `(1,5,7,9)`.
 *
 * @code{.cpp}
 * template<class T, std::size_t... I>
 * auto extract_tuple(T&& t, integer_sequence<std::size_t, I...>)
 *   -> decltype(std::make_tuple(std::get<I>(std::forward<T>(t))...))
 * {
 *  return std::make_tuple(std::get<I>(std::forward<T>(t))...);
 * }
 *
 * int main()
 * {
 *   integer_sequence<std::size_t, 1, 3> seq;
 *   auto a = std::make_tuple(1, 2.0, false, 'a');
 *   auto b = extract_tuple(a, seq);
 *   std::cout << std::get<0>(b) << '\n'; // 2
 *   std::cout << std::get<1>(b) << '\n'; // a
 *   return 0;
 * }
 * @endcode
 */
template<class T, T... N>
class integer_sequence {
};

namespace bits {
  template<class T, long long N, long long... M>
  struct make_integer_sequence :
    make_integer_sequence<T, N-1, N-1, M...>
  {};
  template<class T, long long... M>
  struct make_integer_sequence<T, 0, M...> {
    typedef integer_sequence<T, (T) M...> type;
  };
}

/** A compile-time sequence of integers of the form `(0,1,2,3,...,N-1)` (from C++14) */
template<class T, T N>
using make_integer_sequence = typename simgrid::xbt::bits::make_integer_sequence<T,N>::type;

/** A compile-time sequence of indices (from C++14) */
template<std::size_t... Ints>
using index_sequence = integer_sequence<std::size_t, Ints...>;

/** A compile-time sequence of indices of the form `(0,1,2,3,...,N-1)` (from C++14) */
template<std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

/** Convert a type parameter pack into a index_sequence (from C++14) */
template<class... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

static_assert(std::is_same< make_index_sequence<0>, index_sequence<> >::value, "seq0");
static_assert(std::is_same< make_index_sequence<1>, index_sequence<0> >::value, "seq1");
static_assert(std::is_same< make_index_sequence<2>, index_sequence<0, 1> >::value, "seq2");
static_assert(std::is_same< make_index_sequence<3>, index_sequence<0, 1, 2> >::value, "seq3");
static_assert(std::is_same< index_sequence_for<int,double,float>, make_index_sequence<3> >::value, "seq4");

}
}
#endif
