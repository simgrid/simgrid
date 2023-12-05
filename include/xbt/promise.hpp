/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PROMISE_HPP
#define XBT_PROMISE_HPP

#include <functional>
#include <stdexcept>
#include <variant>
#include <xbt/ex.h>

namespace simgrid::kernel::actor {

/** A value or an exception (or nothing)
 *
 *  This is similar to `optional<expected<T>>`` but it with a Future/Promise like API.
 **/
template <class T> class SimcallResult {
  std::variant<std::monostate, T, std::exception_ptr> value_;

public:
  bool is_valid() const { return value_.index() > 0; }
  void set_exception(std::exception_ptr e) { value_ = std::move(e); }
  void set_value(T&& value) { value_ = std::move(value); }
  void set_value(T const& value) { value_ = value; }

  /** Extract the value from the future. After this, the value is invalid. */
  T get()
  {
    switch (value_.index()) {
      case 1: {
        T value = std::move(std::get<T>(value_));
        value_  = std::monostate();
        return value;
      }
      case 2: {
        std::exception_ptr exception = std::move(std::get<std::exception_ptr>(value_));
        value_                       = std::monostate();
        std::rethrow_exception(std::move(exception));
        break;
      }
      default:
        THROW_IMPOSSIBLE;
    }
  }
};

template <> class SimcallResult<void> : public SimcallResult<std::nullptr_t> {
public:
  void set_value() { SimcallResult<std::nullptr_t>::set_value(nullptr); }
  void get() { SimcallResult<std::nullptr_t>::get(); }
};

template <class T> class SimcallResult<T&> : public SimcallResult<std::reference_wrapper<T>> {
public:
  void set_value(T& value) { SimcallResult<std::reference_wrapper<T>>::set_value(std::ref(value)); }
  T& get() { return SimcallResult<std::reference_wrapper<T>>::get(); }
};

/** Execute some code and set a result accordingly. Takes care of exceptions and works with `void` result.
 *
 *  We might need this when working with generic code because the trivial implementation does not work with `void`
 * (before C++1z).
 *
 *  @param    code  What we want to do
 *  @param  result Where to want to store the result
 */
template <class R, class F> auto exec_simcall(R& result, F&& code) -> decltype(result.set_value(code()))
{
  try {
    result.set_value(std::forward<F>(code)());
  } catch (...) {
    result.set_exception(std::current_exception());
  }
}

template <class R, class F> auto exec_simcall(R& result, F&& code) -> decltype(result.set_value())
{
  try {
    std::forward<F>(code)();
    result.set_value();
  } catch (...) {
    result.set_exception(std::current_exception());
  }
}
} // namespace simgrid::kernel::actor

#endif
