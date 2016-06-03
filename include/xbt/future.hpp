/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUTURE_HPP
#define XBT_FUTURE_HPP

#include <cstddef>

#include <utility>
#include <exception>

namespace simgrid {
namespace xbt {

/** A value or an exception
 *
 *  The API is similar to the one of future and promise.
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

  bool is_valid()
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
class Result<void> : public Result<nullptr_t>
{
public:
  void set_value()
  {
    Result<std::nullptr_t>::set_value(nullptr);
  }
  void get()
  {
    Result<nullptr_t>::get();
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

/** Fulfill a promise by executing a given code */
template<class R, class F>
auto fulfillPromise(R& promise, F&& code)
-> decltype(promise.set_value(code()))
{
  try {
    promise.set_value(code());
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

/** Fulfill a promise by executing a given code
 *
 *  This is a special version for `std::promise<void>` because the default
 *  version does not compile in this case.
 */
template<class P, class F>
auto fulfillPromise(P& promise, F&& code)
-> decltype(promise.set_value())
{
  try {
    (code)();
    promise.set_value();
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

}
}

#endif
