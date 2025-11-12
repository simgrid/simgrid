/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMCALL_HPP
#define SIMGRID_SIMCALL_HPP

#include <simgrid/s4u/Actor.hpp>
#include <xbt/ex.h>
#include <xbt/signal.hpp>

#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

XBT_PUBLIC void simcall_run_answered(std::function<void()> const& code,
                                     simgrid::kernel::actor::SimcallObserver* observer);
XBT_PUBLIC void simcall_run_blocking(std::function<void()> const& code,
                                     simgrid::kernel::actor::SimcallObserver* observer);
XBT_PUBLIC void simcall_run_object_access(std::function<void()> const& code,
                                          simgrid::kernel::actor::ObjectAccessSimcallItem* item);

namespace simgrid::kernel::actor {

/** This class is used to convey a result out of a simcall. It can be a value or an exception (or invalid yet) */
/* Variant for codes returning a value */
template <class F, typename ReturnType = std::invoke_result_t<F>> class SimcallResult {
  std::variant<std::monostate, ReturnType, std::exception_ptr> value_;

public:
  void set_exception(std::exception_ptr e) { value_ = std::move(e); }
  void set_value(ReturnType&& value) { value_ = std::move(value); }
  void set_value(ReturnType const& value) { value_ = value; }

  void exec(F&& code)
  {
    try {
      set_value(code());
    } catch (...) {
      value_ = std::move(std::current_exception());
    }
  }

  /** Extract the value from the future. This invalidates the value, as it's moved away. */
  ReturnType get()
  {
    switch (value_.index()) {
      case 1: { /* Valid value */
        auto val = std::move(std::get<ReturnType>(value_));
        value_   = std::monostate();
        return val;
        break;
      }
      case 2: { /* Exception */
        std::exception_ptr exception = std::move(std::get<std::exception_ptr>(value_));
        value_                       = std::monostate();
        std::rethrow_exception(std::move(exception));
        break;
      }
    }
    THROW_IMPOSSIBLE;
  }
};

/* Variant for void-returning codes*/
template <class F> class SimcallResult<F, void> {
  std::variant<std::monostate, std::exception_ptr> value_;

public:
  void set_exception(std::exception_ptr e) { value_ = std::move(e); }

  void exec(F&& code)
  {
    try {
      code();
    } catch (...) {
      value_ = std::current_exception();
    }
  }

  /** Extract the value from the future. This invalidates the value, as it's moved away. */
  void get()
  {
    switch (value_.index()) {
      case 0: /* No exception */
        break;
      case 1: {
        std::exception_ptr exception = std::move(std::get<std::exception_ptr>(value_));
        value_                       = std::monostate();
        std::rethrow_exception(std::move(exception));
        break;
      }
    }
  }
};

/** Execute some code in kernel context on behalf of the user code.
 *
 * Every modification of the environment must be protected this way: every setter, constructor and similar.
 * Getters don't have to be protected this way, and setters may use the simcall_object_access() variant (see below).
 *
 * This allows deterministic parallel simulation without any locking, even if almost nobody uses parallel simulation in
 * SimGrid. More interestingly it makes every modification of the simulated world observable by the model-checker,
 * allowing the whole MC business.
 *
 * It is highly inspired from the syscalls in a regular operating system, allowing the user code to get some specific
 * code executed in the kernel context. But here, there is almost no security involved. Parameters get checked for
 * finiteness but that's all. The main goal remain to ensure reproducible ordering of uncomparable events (in
 * [parallel] simulation) and observability of events (in model-checking).
 *
 * The code passed as argument is supposed to terminate at the exact same simulated timestamp.
 * Do not use it if your code may block waiting for a subsequent event, e.g. if you lock a mutex,
 * you may need to wait for that mutex to be unlocked by its current owner.
 * Potentially blocking simcall must be issued using simcall_blocking(), right below in this file.
 */
template <class F> typename std::invoke_result_t<F> simcall_answered(F&& code, SimcallObserver* observer = nullptr)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall marshalling/unmarshalling/dispatch:
  if (s4u::Actor::is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which executes it for us and reports the result.
  // We use a SimcallResult which conveniently handles the success/failure value for us.
  using R = typename std::invoke_result_t<F>;
  SimcallResult<F> result;
  simcall_run_answered([&result, &code] { result.exec(std::forward<F>(code)); }, observer);
  return result.get();
}

/** Use a setter on the `item` object. That's a simcall only if running in parallel or with MC activated.
 *
 * Simulation without MC and without parallelism (contexts/nthreads=1) will not pay the price of a simcall for an
 * harmless setter. When running in parallel, you want your write access to be done in a mutual exclusion way, while the
 * getters can still occur out of order.
 *
 * When running in MC, you want to make this access visible to the checker. Actually in this case, it's not visible from
 * the checker (and thus still use a fast track) if the setter is called from the actor that created the object.
 */
template <class F> typename std::invoke_result_t<F> simcall_object_access(ObjectAccessSimcallItem* item, F&& code)
{
  // If we are in the maestro, we take the fast path and execute the code directly
  if (simgrid::s4u::Actor::is_maestro())
    return std::forward<F>(code)();

  // If called from another thread, do a real simcall. It will be short-cut on need
  SimcallResult<F> result;
  simcall_run_object_access([&result, &code] { result.exec(std::forward<F>(code)); }, item);

  return result.get();
}

/** Execute some code (that does not return immediately) in kernel context
 *
 * This is very similar to simcall_answered() above, but the calling actor will not get rescheduled automatically.
 *
 * This is meant for blocking actions. For example, locking a mutex is a blocking simcall. First it's a simcall because
 * that's obviously a modification of the world. Then, that's a blocking simcall because if the mutex happens not to be
 * free, the actor is added to a queue of actors in the mutex. Every mutex->unlock() takes the first actor from the
 * queue, mark it as current owner of the mutex and call actor->simcall_answer() on it to mark that this actor is now
 * unblocked and ready to run again. If the mutex is initially free, the calling actor is unblocked right away with
 * actor->simcall_answer() once the mutex is marked as locked.
 *
 * If your simcall handler (the F parameter) never calls actor->simcall_answer() by itself, the actor will never return
 * from this simcall.
 *
 * The return value is obtained from observer->get_result(), so your code should call observer->set_value() before
 * simcall_answer(), unless this is a synchronization-only simcall with no return value (i.e if ReturnType=void)
 */
template <typename ReturnType, class F, typename = std::enable_if_t<std::is_void_v<std::invoke_result_t<F>>>>
ReturnType simcall_blocking(F&& code, DelayedSimcallObserver<ReturnType>* observer)
{
  xbt_assert(not s4u::Actor::is_maestro(), "Cannot execute blocking call in kernel mode");

  // Pass the code to the maestro which executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  SimcallResult<F> result;
  simcall_run_blocking([&result, &code] { result.exec(std::forward<F>(code)); }, observer);
  result.get(); // rethrow stored exception if any
  return observer->get_result();
}
} // namespace simgrid::kernel::actor
#endif
