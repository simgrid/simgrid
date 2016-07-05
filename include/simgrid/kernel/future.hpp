/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_FUTURE_HPP
#define SIMGRID_KERNEL_FUTURE_HPP

#include <functional>
#include <future>
#include <memory>
#include <utility>
#include <type_traits>

#include <boost/optional.hpp>

#include <xbt/base.h>
#include <xbt/functional.hpp>
#include <xbt/future.hpp>

namespace simgrid {
namespace kernel {

// There are the public classes:
template<class T> class Future;
template<class T> class Promise;

// Those are implementation details:
enum class FutureStatus;
template<class T> class FutureState;

enum class FutureStatus {
  not_ready,
  ready,
  done,
};

template<class T>
struct is_future : std::false_type {};
template<class T>
struct is_future<Future<T>> : std::true_type {};

/** Bases stuff for all @ref simgrid::kernel::FutureState<T> */
class FutureStateBase {
public:
  // No copy/move:
  FutureStateBase(FutureStateBase const&) = delete;
  FutureStateBase& operator=(FutureStateBase const&) = delete;

  XBT_PUBLIC(void) schedule(simgrid::xbt::Task<void()>&& job);

  void set_exception(std::exception_ptr exception)
  {
    xbt_assert(exception_ == nullptr);
    if (status_ != FutureStatus::not_ready)
      throw std::future_error(std::future_errc::promise_already_satisfied);
    exception_ = std::move(exception);
    this->set_ready();
  }

  void set_continuation(simgrid::xbt::Task<void()>&& continuation)
  {
    xbt_assert(!continuation_);
    switch (status_) {
    case FutureStatus::done:
      // This is not supposed to happen if continuation is set
      // via the Promise:
      xbt_die("Set continuation on finished future");
      break;
    case FutureStatus::ready:
      // The future is ready, execute the continuation directly.
      // We might execute it from the event loop instead:
      schedule(std::move(continuation));
      break;
    case FutureStatus::not_ready:
      // The future is not ready so we mast keep the continuation for
      // executing it later:
      continuation_ = std::move(continuation);
      break;
    default:
      DIE_IMPOSSIBLE;
    }
  }

  FutureStatus get_status() const
  {
    return status_;
  }

  bool is_ready() const
  {
    return status_ == FutureStatus::ready;
  }

protected:
  FutureStateBase() = default;
  ~FutureStateBase() = default;

  /** Set the future as ready and trigger the continuation */
  void set_ready()
  {
    status_ = FutureStatus::ready;
    if (continuation_) {
      // We unregister the continuation before executing it.
      // We need to do this becase the current implementation of the
      // continuation has a shared_ptr to the FutureState.
      auto continuation = std::move(continuation_);
      this->schedule(std::move(continuation));
    }
  }

  /** Set the future as done and raise an exception if any
   *
   *  This does half the job of `.get()`.
   **/
  void resolve()
  {
    if (status_ != FutureStatus::ready)
      xbt_die("Deadlock: this future is not ready");
    status_ = FutureStatus::done;
    if (exception_) {
      std::exception_ptr exception = std::move(exception_);
      std::rethrow_exception(std::move(exception));
    }
  }

private:
  FutureStatus status_ = FutureStatus::not_ready;
  std::exception_ptr exception_;
  simgrid::xbt::Task<void()> continuation_;
};

/** Shared state for future and promises
 *
 *  You are not expected to use them directly but to create them
 *  implicitely through a @ref simgrid::kernel::Promise.
 *  Alternatively kernel operations could inherit or contain FutureState
 *  if they are managed with @ref std::shared_ptr.
 **/
template<class T>
class FutureState : public FutureStateBase {
public:

  void set_value(T value)
  {
    if (this->get_status() != FutureStatus::not_ready)
      throw std::future_error(std::future_errc::promise_already_satisfied);
    value_ = std::move(value);
    this->set_ready();
  }

  T get()
  {
    this->resolve();
    xbt_assert(this->value_);
    auto result = std::move(this->value_.get());
    this->value_ = boost::optional<T>();
    return std::move(result);
  }

private:
  boost::optional<T> value_;
};

template<class T>
class FutureState<T&> : public FutureStateBase {
public:
  void set_value(T& value)
  {
    if (this->get_status() != FutureStatus::not_ready)
      throw std::future_error(std::future_errc::promise_already_satisfied);
    value_ = &value;
    this->set_ready();
  }

  T& get()
  {
    this->resolve();
    xbt_assert(this->value_);
    T* result = value_;
    value_ = nullptr;
    return *value_;
  }

private:
  T* value_ = nullptr;
};

template<>
class FutureState<void> : public FutureStateBase {
public:
  void set_value()
  {
    if (this->get_status() != FutureStatus::not_ready)
      throw std::future_error(std::future_errc::promise_already_satisfied);
    this->set_ready();
  }

  void get()
  {
    this->resolve();
  }
};

template<class T>
void bindPromise(Promise<T> promise, Future<T> future)
{
  struct PromiseBinder {
  public:
    PromiseBinder(Promise<T> promise) : promise_(std::move(promise)) {}
    void operator()(Future<T> future)
    {
      simgrid::xbt::setPromise(promise_, future);
    }
  private:
    Promise<T> promise_;
  };
  future.then_(PromiseBinder(std::move(promise)));
}

template<class T> Future<T> unwrapFuture(Future<Future<T>> future);

/** Result of some (probably) asynchronous operation in the SimGrid kernel
 *
 * @ref simgrid::simix::Future and @ref simgrid::simix::Future provide an
 * abstration for asynchronous stuff happening in the SimGrid kernel. They
 * are based on C++1z futures.
 *
 * The future represents a value which will be available at some point when this
 * asynchronous operaiont is finished. Alternatively, if this operations fails,
 * the result of the operation might be an exception.
 *
 *  As the operation is possibly no terminated yet, we cannot get the result
 *  yet. Moreover, as we cannot block in the SimGrid kernel we cannot wait for
 *  it. However, we can attach some code/callback/continuation which will be
 *  executed when the operation terminates.
 *
 *  Example of the API (`simgrid::kernel::createProcess` does not exist):
 *  <pre>
 *  // Create a new process using the Worker code, this process returns
 *  // a std::string:
 *  simgrid::kernel::Future<std::string> future =
 *     simgrid::kernel::createProcess("worker42", host, Worker(42));
 *  // At this point, we just created the process so the result is not available.
 *  // However, we can attach some work do be done with this result:
 *  future.then([](simgrid::kernel::Future<std::string> result) {
 *    // This code is called when the operation is completed so the result is
 *    // available:
 *    try {
 *      // Try to get value, this might throw an exception if the operation
 *      // failed (such as an exception throwed by the worker process):
 *      std::string value = result.get();
 *      XBT_INFO("Value: %s", value.c_str());
 *    }
 *    catch(std::exception& e) {
 *      // This is an exception from the asynchronous operation:
 *      XBT_INFO("Error: %e", e.what());
 *    }
 *  );
 *  </pre>
 *
 *  This is based on C++1z @ref std::future but with some differences:
 *
 *  * there is no thread synchronization (atomic, mutex, condition variable,
 *    etc.) because everything happens in the SimGrid event loop;
 *
 *  * it is purely asynchronous, you are expected to use `.then()`;
 *
 *  * inside the `.then()`, `.get()` can be used;
 *
 *  * `.get()` can only be used when `.is_ready()` (as everything happens in
 *     a single-thread, the future would be guaranted to deadlock if `.get()`
 *     is called when the future is not ready);
 *
 *  * there is no future chaining support for now (`.then().then()`);
 *
 *  * there is no sharing (`shared_future`) for now.
 */
template<class T>
class Future {
public:
  Future() = default;
  Future(std::shared_ptr<FutureState<T>> state): state_(std::move(state)) {}

  // Move type:
  Future(Future&) = delete;
  Future& operator=(Future&) = delete;
  Future(Future&& that) : state_(std::move(that.state_)) {}
  Future& operator=(Future&& that)
  {
    state_ = std::move(that.state_);
    return *this;
  }

  /** Whether the future is valid:.
   *
   *  A future which as been used (`.then` of `.get`) becomes invalid.
   *
   *  We can use `.then` on a valid future.
   */
  bool valid() const
  {
    return state_ != nullptr;
  }

  /** Whether the future is ready
   *
   *  A future is ready when it has an associated value or exception.
   *
   *  We can use `.get()` on ready futures.
   **/
  bool is_ready() const
  {
    return state_ != nullptr && state_->is_ready();
  }

  /** Attach a continuation to this future
   *
   *  This is like .then() but avoid the creation of a new future.
   */
  template<class F>
  void then_(F continuation)
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    // Give shared-ownership to the continuation:
    auto state = std::move(state_);
    state->set_continuation(simgrid::xbt::makeTask(
      std::move(continuation), state));
  }

  /** Attach a continuation to this future
   *
   *  This version never does future unwrapping.
   */
  template<class F>
  auto thenNoUnwrap(F continuation)
  -> Future<decltype(continuation(std::move(*this)))>
  {
    typedef decltype(continuation(std::move(*this))) R;
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    auto state = std::move(state_);
    // Create a new future...
    Promise<R> promise;
    Future<R> future = promise.get_future();
    // ...and when the current future is ready...
    state->set_continuation(simgrid::xbt::makeTask(
      [](Promise<R> promise, std::shared_ptr<FutureState<T>> state, F continuation) {
        // ...set the new future value by running the continuation.
        Future<T> future(std::move(state));
        simgrid::xbt::fulfillPromise(promise,[&]{
          return continuation(std::move(future));
        });
      },
      std::move(promise), state, std::move(continuation)));
    return std::move(future);
  }

  /** Attach a continuation to this future
   *
   *  The future must be valid in order to make this call.
   *  The continuation is executed when the future becomes ready.
   *  The future becomes invalid after this call.
   *
   * @param continuation This function is called with a ready future
   *                     the future is ready
   * @exception std::future_error no state is associated with the future
   */
  template<class F>
  auto then(F continuation)
  -> typename std::enable_if<
       !is_future<decltype(continuation(std::move(*this)))>::value,
       Future<decltype(continuation(std::move(*this)))>
     >::type
  {
    return this->thenNoUnwrap(std::move(continuation));
  }

  /** Attach a continuation to this future (future chaining) */
  template<class F>
  auto then(F continuation)
  -> typename std::enable_if<
       is_future<decltype(continuation(std::move(*this)))>::value,
       decltype(continuation(std::move(*this)))
     >::type
  {
    return unwrapFuture(this->thenNoUnwap(std::move(continuation)));
  }

  /** Get the value from the future
   *
   *  This is expected to be called
   *
   *  The future must be valid and ready in order to make this call.
   *  @ref std::future blocks when the future is not ready but we are
   *  completely single-threaded so blocking would be a deadlock.
   *  After the call, the future becomes invalid.
   *
   *  @return                      value of the future
   *  @exception any               Exception from the future
   *  @exception std::future_error no state is associated with the future
   */
  T get()
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    std::shared_ptr<FutureState<T>> state = std::move(state_);
    return state->get();
  }

private:
  std::shared_ptr<FutureState<T>> state_;
};

template<class T>
Future<T> unwrapFuture(Future<Future<T>> future)
{
  Promise<T> promise;
  Future<T> result = promise.get_future();
  bindPromise(std::move(promise), std::move(future));
  return std::move(result);
}

/** Producer side of a @simgrid::kernel::Future
 *
 *  A @ref Promise is connected to some `Future` and can be used to
 *  set its result.
 *
 *  Similar to @ref std::promise
 *
 *  <code>
 *  // Create a promise and a future:
 *  auto promise = std::make_shared<simgrid::kernel::Promise<T>>();
 *  auto future = promise->get_future();
 *
 *  SIMIX_timer_set(date, [promise] {
 *    try {
 *      int value = compute_the_value();
 *      if (value < 0)
 *        throw std::logic_error("Bad value");
 *      // Whenever the operation is completed, we set the value
 *      // for the future:
 *      promise.set_value(value);
 *    }
 *    catch (...) {
 *      // If an error occured, we can set an exception which
 *      // will be throwed buy future.get():
 *      promise.set_exception(std::current_exception());
 *    }
 *  });
 *
 *  // Return the future to the caller:
 *  return future;
 *  </code>
 **/
template<class T>
class Promise {
public:
  Promise() : state_(std::make_shared<FutureState<T>>()) {}
  Promise(std::shared_ptr<FutureState<T>> state) : state_(std::move(state)) {}

  // Move type
  Promise(Promise const&) = delete;
  Promise& operator=(Promise const&) = delete;
  Promise(Promise&& that) :
    state_(std::move(that.state_)), future_get_(that.future_get_)
  {
    that.future_get_ = false;
  }

  Promise& operator=(Promise&& that)
  {
    this->state_ = std::move(that.state_);
    this->future_get_ = that.future_get_;
    that.future_get_ = false;
    return *this;
  }
  Future<T> get_future()
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    if (future_get_)
      throw std::future_error(std::future_errc::future_already_retrieved);
    future_get_ = true;
    return Future<T>(state_);
  }
  void set_value(T value)
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    state_->set_value(std::move(value));
  }
  void set_exception(std::exception_ptr exception)
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    state_->set_exception(std::move(exception));
  }
  ~Promise()
  {
    if (state_ && state_->get_status() == FutureStatus::not_ready)
      state_->set_exception(std::make_exception_ptr(
        std::future_error(std::future_errc::broken_promise)));
  }

private:
  std::shared_ptr<FutureState<T>> state_;
  bool future_get_ = false;
};

template<>
class Promise<void> {
public:
  Promise() : state_(std::make_shared<FutureState<void>>()) {}
  Promise(std::shared_ptr<FutureState<void>> state) : state_(std::move(state)) {}
  ~Promise()
  {
    if (state_ && state_->get_status() == FutureStatus::not_ready)
      state_->set_exception(std::make_exception_ptr(
        std::future_error(std::future_errc::broken_promise)));
  }

  // Move type
  Promise(Promise const&) = delete;
  Promise& operator=(Promise const&) = delete;
  Promise(Promise&& that) :
    state_(std::move(that.state_)), future_get_(that.future_get_)
  {
    that.future_get_ = false;
  }
  Promise& operator=(Promise&& that)
  {
    this->state_ = std::move(that.state_);
    this->future_get_ = that.future_get_;
    that.future_get_ = false;
    return *this;
  }

  Future<void> get_future()
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    if (future_get_)
      throw std::future_error(std::future_errc::future_already_retrieved);
    future_get_ = true;
    return Future<void>(state_);
  }
  void set_value()
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    state_->set_value();
  }
  void set_exception(std::exception_ptr exception)
  {
    if (state_ == nullptr)
      throw std::future_error(std::future_errc::no_state);
    state_->set_exception(std::move(exception));
  }

private:
  std::shared_ptr<FutureState<void>> state_;
  bool future_get_ = false;
};

}
}

#endif
