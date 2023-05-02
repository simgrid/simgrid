/* Copyright (c) 2004-2023 The SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_UTILS_ITER_ITERATOR_WRAPPING_HPP
#define XBT_UTILS_ITER_ITERATOR_WRAPPING_HPP

#include <tuple>
#include <type_traits>

namespace simgrid::xbt {

template <typename T> struct ref_or_value {
  using type =
      std::conditional_t<std::is_lvalue_reference_v<T>, std::reference_wrapper<typename std::remove_reference_t<T>>, T>;
};
template <typename T> using ref_or_value_t = typename ref_or_value<T>::type;

/**
 * @brief A container which simply forwards its arguments to an
 * iterator to begin traversal over another collection
 *
 * An `iterator_wrapping` acts as a proxy to the collection that it
 * wraps. You create an `iterator_wrapping` as a convenience to needing
 * to manually check stop and end conditions when constructing iterators
 * directly, e.g. in a for-loop. With an `iterator_wrapping`, you can
 * simply act as if the iterator were a collection and use it e.g. in
 * auto-based for-loops.
 *
 * @class Iterator: The type of the iterator that is constructed to
 * traverse the container.
 *
 * @note The wrapping only works correctly if the iterator type (Iterator)
 * that is constructed can be default constructed, and only if the default-constructed
 * iterate is a valid representation of the "end()" of any iterator type.
 * That is, if you must supply additional arguments to indicate the end of a collection,
 * you'll have to construct is manually.
 */
template <typename Iterator, typename... Args> struct iterator_wrapping {
private:
  std::tuple<ref_or_value_t<Args>...> m_args;

  template <typename IteratorType, typename... Arguments>
  friend constexpr iterator_wrapping<IteratorType, Arguments...> make_iterator_wrapping(Arguments&&... args);

  template <typename IteratorType, typename... Arguments>
  friend constexpr iterator_wrapping<IteratorType, Arguments...> make_iterator_wrapping_explicit(Arguments... args);

public:
  iterator_wrapping(Args&&... begin_iteration) : m_args(ref_or_value_t<Args>(begin_iteration)...) {}
  iterator_wrapping(const iterator_wrapping&)            = delete;
  iterator_wrapping(iterator_wrapping&&)                 = delete;
  iterator_wrapping& operator=(const iterator_wrapping&) = delete;
  iterator_wrapping& operator=(iterator_wrapping&&)      = delete;

  Iterator begin() const
  {
    return std::apply([](auto&... args) { return Iterator(args...); }, m_args);
  }
  Iterator end() const { return Iterator(); }
};

template <typename Iterator, typename... Args>
constexpr iterator_wrapping<Iterator, Args...> make_iterator_wrapping(Args&&... args)
{
  return iterator_wrapping<Iterator, Args...>(std::forward<Args>(args)...);
}

template <typename Iterator, typename... Args>
constexpr iterator_wrapping<Iterator, Args...> make_iterator_wrapping_explicit(Args... args)
{
  return iterator_wrapping<Iterator, Args...>(args...);
}

} // namespace simgrid::xbt

#endif
