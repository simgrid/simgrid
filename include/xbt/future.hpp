/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUTURE_HPP
#define XBT_FUTURE_HPP

#include <cstddef>

#include <exception>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace simgrid {
namespace xbt {

/** A value or an exception (or nothing)
 *
 *  This is similar to optional<expected<T>> but it with a Future/Promise
 *  like API.
 *
 *  Also the name it not so great.
 **/
template<class T>
class Result {
  enum class ResultStatus {
    invalid,
    value,
    exception,
  };
public:
  Result() {}
  ~Result() { this->reset(); }

  // Copy (if T is copyable) and move:
  Result(Result const& that)
  {
    (*this) = that;
  }
  Result& operator=(Result const& that)
  {
    this->reset();
    switch (that.status_) {
      case ResultStatus::invalid:
        break;
      case ResultStatus::valid:
        new (&value_) T(that.value);
        break;
      case ResultStatus::exception:
        new (&exception_) T(that.exception);
        break;
    }
    return *this;
  }
  Result(Result&& that)
  {
    *this = std::move(that);
  }
  Result& operator=(Result&& that)
  {
    this->reset();
    switch (that.status_) {
      case ResultStatus::invalid:
        break;
      case ResultStatus::valid:
        new (&value_) T(std::move(that.value));
        that.value.~T();
        break;
      case ResultStatus::exception:
        new (&exception_) T(std::move(that.exception));
        that.exception.~exception_ptr();
        break;
    }
    that.status_ = ResultStatus::invalid;
    return *this;
  }

  bool is_valid() const
  {
    return status_ != ResultStatus::invalid;
  }
  void reset()
  {
    switch (status_) {
      case ResultStatus::invalid:
        break;
      case ResultStatus::value:
        value_.~T();
        break;
      case ResultStatus::exception:
        exception_.~exception_ptr();
        break;
    }
    status_ = ResultStatus::invalid;
  }
  void set_exception(std::exception_ptr e)
  {
    this->reset();
    new (&exception_) std::exception_ptr(std::move(e));
    status_ = ResultStatus::exception;
  }
  void set_value(T&& value)
  {
    this->reset();
    new (&value_) T(std::move(value));
    status_ = ResultStatus::value;
  }
  void set_value(T const& value)
  {
    this->reset();
    new (&value_) T(value);
    status_ = ResultStatus::value;
  }

  /** Extract the value from the future
   *
   *  After this the value is invalid.
   **/
  T get()
  {
    switch (status_) {
      case ResultStatus::invalid:
      default:
        throw std::logic_error("Invalid result");
      case ResultStatus::value: {
        T value = std::move(value_);
        value_.~T();
        status_ = ResultStatus::invalid;
        return std::move(value);
      }
      case ResultStatus::exception: {
        std::exception_ptr exception = std::move(exception_);
        exception_.~exception_ptr();
        status_ = ResultStatus::invalid;
        std::rethrow_exception(std::move(exception));
        break;
      }
    }
  }
private:
  ResultStatus status_ = ResultStatus::invalid;
  union {
    T value_;
    std::exception_ptr exception_;
  };
};

template<>
class Result<void> : public Result<std::nullptr_t>
{
public:
  void set_value()
  {
    Result<std::nullptr_t>::set_value(nullptr);
  }
  void get()
  {
    Result<std::nullptr_t>::get();
  }
};

template<class T>
class Result<T&> : public Result<std::reference_wrapper<T>>
{
public:
  void set_value(T& value)
  {
    Result<std::reference_wrapper<T>>::set_value(std::ref(value));
  }
  T& get()
  {
    return Result<std::reference_wrapper<T>>::get();
  }
};

/** Execute some code and set a promise or result accordingly
 *
 *  Roughly this does:
 *
 *  <pre>
 *  promise.set_value(code());
 *  </pre>
 *
 *  but it takes care of exceptions and works with void.
 *
 *  We might need this when working with generic code because
 *  the trivial implementation does not work with `void` (before C++1z).
 *
 *  @param    code  What we want to do
 *  @param  promise Where to want to store the result
 */
template<class R, class F>
auto fulfillPromise(R& promise, F&& code)
-> decltype(promise.set_value(code()))
{
  try {
    promise.set_value(std::forward<F>(code)());
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

template<class P, class F>
auto fulfillPromise(P& promise, F&& code)
-> decltype(promise.set_value())
{
  try {
    std::forward<F>(code)();
    promise.set_value();
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

/** Set a promise/result from a future/resul
 *
 *  Roughly this does:
 *
 *  <pre>promise.set_value(future);</pre>
 *
 *  but it takes care of exceptions and works with `void`.
 *
 *  We might need this when working with generic code because
 *  the trivial implementation does not work with `void` (before C++1z).
 *
 *  @param promise output (a valid future or a result)
 *  @param future  input (a ready/waitable future or a valid result)
 */
template<class P, class F> inline
void setPromise(P& promise, F&& future)
{
  fulfillPromise(promise, [&]{ return std::forward<F>(future).get(); });
}

}
}

#endif
