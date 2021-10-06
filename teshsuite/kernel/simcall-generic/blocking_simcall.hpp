/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP
#define SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP

#include <exception>
#include <functional>
#include <future>
#include <utility>

#include <xbt/sysdep.h>

#include <simgrid/kernel/future.hpp>
#include <xbt/promise.hpp>

#include "src/kernel/actor/ActorImpl.hpp"

namespace simgrid {
namespace simix {

/** Execute some code in kernel mode and wakes up the actor when
 *  the result is available.
 *
 * It is given a callback which is executed in the SimGrid kernel and
 * returns a `simgrid::kernel::Future<T>`. The kernel blocks the actor
 * until the Future is ready and:
 *
 *  - either returns the value wrapped in the future to the actor
 *
 *  - or raises the exception stored in the future in the actor.
 *
 * This can be used to implement blocking calls without adding new simcalls.
 * One downside of this approach is that we don't have any semantic on what
 * the actor is waiting. This might be a problem for the model-checker and
 * we'll have to devise a way to make it work.
 *
 * @param     code Kernel code returning a `simgrid::kernel::Future<T>`
 * @return         Value of the kernel future
 * @exception      Exception from the kernel future
 */
template <class F> auto kernel_sync(F code) -> decltype(code().get())
{
  using T = decltype(code().get());
  xbt_assert(not s4u::Actor::is_maestro(), "Cannot execute blocking call in kernel mode");

  auto self = kernel::actor::ActorImpl::self();
  xbt::Result<T> result;
  simcall_run_blocking(
      [&result, self, &code] {
        try {
          auto future = code();
          future.then_([&result, self](std::shared_ptr<kernel::FutureState<T>> value) {
            xbt::set_promise(result, kernel::Future<T>(std::move(value)));
            unblock(self);
          });
        } catch (...) {
          result.set_exception(std::current_exception());
          unblock(self);
        }
      },
      nullptr);
  return result.get();
}

/** A blocking (`wait()`-based) future for SIMIX processes */
// TODO:
// - .wait_for
// - .wait_until
// - SharedFuture
// - simgrid::simix::when_all - wait for all future to be ready (this one is simple!)
// - simgrid::simix::when_any - wait for any future to be ready
template <class T> class Future {
public:
  Future() = default;
  explicit Future(simgrid::kernel::Future<T> future) : future_(std::move(future)) {}
  Future(Future&&) noexcept = default;
  Future& operator=(Future&&) noexcept = default;

  bool valid() const { return future_.valid(); }
  T get()
  {
    if (not valid())
      throw std::future_error(std::future_errc::no_state);
    auto self = kernel::actor::ActorImpl::self();
    xbt::Result<T> result;
    simcall_run_blocking(
        [this, &result, self] {
          try {
            // When the kernel future is ready...
            this->future_.then_([&result, self](std::shared_ptr<kernel::FutureState<T>> value) {
              // ... wake up the process with the result of the kernel future.
              xbt::set_promise(result, kernel::Future<T>(std::move(value)));
              unblock(self);
            });
          } catch (...) {
            result.set_exception(std::current_exception());
            unblock(self);
          }
        },
        nullptr);
    return result.get();
  }
  bool is_ready() const
  {
    if (not valid())
      throw std::future_error(std::future_errc::no_state);
    return future_.is_ready();
  }
  void wait()
  {
    // The future is ready! We don't have to wait:
    if (this->is_ready())
      return;
    // The future is not ready. We have to delegate to the SimGrid kernel:
    std::exception_ptr exception;
    auto self = kernel::actor::ActorImpl::self();
    simcall_run_blocking(
        [this, &exception, self] {
          try {
            // When the kernel future is ready...
            this->future_.then_([this, self](std::shared_ptr<kernel::FutureState<T>> value) {
              // ...store it the simix kernel and wake up.
              this->future_ = kernel::Future<T>(std::move(value));
              unblock(self);
            });
          } catch (...) {
            exception = std::current_exception();
            unblock(self);
          }
        },
        nullptr);
  }

private:
  // We wrap an event-based kernel future:
  kernel::Future<T> future_;
};

/** Start some asynchronous work
 *
 *  @param code SimGrid kernel code which returns a simgrid::kernel::Future
 *  @return     Actor future
 */
template <class F> auto kernel_async(F code) -> Future<decltype(code().get())>
{
  using T = decltype(code().get());

  // Execute the code in the kernel and get the kernel future:
  kernel::Future<T> future = kernel::actor::simcall(std::move(code));

  // Wrap the kernel future in an actor future:
  return Future<T>(std::move(future));
}
} // namespace simix
} // namespace simgrid

#endif
