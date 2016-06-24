/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP
#define SIMGRID_SIMIX_BLOCKING_SIMCALL_HPP

#include <exception>
#include <functional>
#include <future>
#include <utility>

#include <xbt/sysdep.h>

#include <xbt/future.hpp>
#include <simgrid/kernel/future.hpp>
#include <simgrid/simix.h>
#include <simgrid/simix.hpp>

XBT_PUBLIC(void) simcall_run_blocking(std::function<void()> const& code);

namespace simgrid {
namespace simix {

XBT_PUBLIC(void) unblock(smx_process_t process);

/** Execute some code in kernel mode and wakes up the process when
 *  the result is available.
 *
 * It is given a callback which is executed in the kernel SimGrid and
 * returns a simgrid::kernel::Future<T>. The kernel blocks the process
 * until the Future is ready and either the value wrapped in the future
 * to the process or raises the exception stored in the Future in the process.
 *
 * This can be used to implement blocking calls without adding new simcalls.
 * One downside of this approach is that we don't have any semantic on what
 * the process is waiting. This might be a problem for the model-checker and
 * we'll have to devise a way to make it work.
 *
 * @param     code Kernel code returning a `simgrid::kernel::Future<T>`
 * @return         Value of the kernel future
 * @exception      Exception from the kernel future
 */
template<class F>
auto kernelSync(F code) -> decltype(code().get())
{
  typedef decltype(code().get()) T;
  if (SIMIX_is_maestro())
    xbt_die("Can't execute blocking call in kernel mode");

  smx_process_t self = SIMIX_process_self();
  simgrid::xbt::Result<T> result;

  simcall_run_blocking([&result, self, &code]{
    try {
      auto future = code();
      future.then_([&result, self](simgrid::kernel::Future<T> value) {
        simgrid::xbt::setPromise(result, value);
        simgrid::simix::unblock(self);
      });
    }
    catch (...) {
      result.set_exception(std::current_exception());
      simgrid::simix::unblock(self);
    }
  });
  return result.get();
}

/** A blocking (`wait()`-based) future for SIMIX processes */
// TODO, .wait_for()
// TODO, .wait_until()
// TODO, SharedFuture
// TODO, simgrid::simix::when_all - wait for all future to be ready (this one is simple!)
// TODO, simgrid::simix::when_any - wait for any future to be ready
template <class T>
class Future {
public:
  Future() {}
  Future(simgrid::kernel::Future<T> future) : future_(std::move(future)) {}

  bool valid() const { return future_.valid(); }
  T get()
  {
    if (!valid())
      throw std::future_error(std::future_errc::no_state);
    smx_process_t self = SIMIX_process_self();
    simgrid::xbt::Result<T> result;
    simcall_run_blocking([this, &result, self]{
      try {
        // When the kernel future is ready...
        this->future_.then_([this, &result, self](simgrid::kernel::Future<T> value) {
          // ... wake up the process with the result of the kernel future.
          simgrid::xbt::setPromise(result, value);
          simgrid::simix::unblock(self);
        });
      }
      catch (...) {
        result.set_exception(std::current_exception());
        simgrid::simix::unblock(self);
      }
    });
    return result.get();
  }
  bool is_ready() const
  {
    if (!valid())
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
    smx_process_t self = SIMIX_process_self();
    simcall_run_blocking([this, &exception, self]{
      try {
        // When the kernel future is ready...
        this->future_.then_([this, self](simgrid::kernel::Future<T> value) {
          // ...store it the simix kernel and wake up.
          this->future_ = std::move(value);
          simgrid::simix::unblock(self);
        });
      }
      catch (...) {
        exception = std::current_exception();
        simgrid::simix::unblock(self);
      }
    });
  }
private:
  // We wrap an event-based kernel future:
  simgrid::kernel::Future<T> future_;
};

/** Start some asynchronous work
 *
 *  @param code SimGrid kernel code which returns a simgrid::kernel::Future
 *  @return     User future
 */
template<class F>
auto kernelAsync(F code)
  -> Future<decltype(code().get())>
{
  typedef decltype(code().get()) T;

  // Execute the code in the kernel and get the kernel simcall:
  simgrid::kernel::Future<T> future =
    simgrid::simix::kernelImmediate(std::move(code));

  // Wrap tyhe kernel simcall in a user simcall:
  return simgrid::simix::Future<T>(std::move(future));
}

}
}

#endif
