/* Copyright (c) 2007-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_HPP
#define SIMGRID_SIMIX_HPP

#include <simgrid/s4u/Actor.hpp>
#include <simgrid/simix.h>
#include <xbt/promise.hpp>
#include <xbt/signal.hpp>

#include <string>
#include <unordered_map>

XBT_PUBLIC void simcall_run_answered(std::function<void()> const& code,
                                     simgrid::kernel::actor::SimcallObserver* observer);
XBT_PUBLIC void simcall_run_blocking(std::function<void()> const& code,
                                     simgrid::kernel::actor::SimcallObserver* observer);

namespace simgrid {
namespace kernel {
namespace actor {

/** Execute some code in kernel context on behalf of the user code.
 *
 * Every modification of the environment must be protected this way: every setter, constructor and similar.
 * Getters don't have to be protected this way.
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
template <class F> typename std::result_of_t<F()> simcall_answered(F&& code, SimcallObserver* observer = nullptr)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall marshalling/unmarshalling/dispatch:
  if (s4u::Actor::is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  using R = typename std::result_of_t<F()>;
  simgrid::xbt::Result<R> result;
  simcall_run_answered([&result, &code] { simgrid::xbt::fulfill_promise(result, std::forward<F>(code)); }, observer);
  return result.get();
}

/** Execute some code (that does not return immediately) in kernel context
 *
 * This is very similar to simcall() right above, but the calling actor will not get rescheduled until
 * actor->simcall_answer() is called explicitly.
 *
 * This is meant for blocking actions. For example, locking a mutex is a blocking simcall.
 * First it's a simcall because that's obviously a modification of the world. Then, that's a blocking simcall because if
 * the mutex happens not to be free, the actor is added to a queue of actors in the mutex. Every mutex->unlock() takes
 * the first actor from the queue, mark it as current owner of the mutex and call actor->simcall_answer() to mark that
 * this mutex is now unblocked and ready to run again. If the mutex is initially free, the calling actor is unblocked
 * right away with actor->simcall_answer() once the mutex is marked as locked.
 *
 * If your code never calls actor->simcall_answer() itself, the actor will never return from its simcall.
 *
 * The return value is obtained from observer->get_result() if it exists. Otherwise void is returned.
 */
template <class F> void simcall_blocking(F&& code, SimcallObserver* observer = nullptr)
{
  xbt_assert(not s4u::Actor::is_maestro(), "Cannot execute blocking call in kernel mode");

  // Pass the code to the maestro which executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  simgrid::xbt::Result<void> result;
  simcall_run_blocking([&result, &code] { simgrid::xbt::fulfill_promise(result, std::forward<F>(code)); }, observer);
  result.get(); // rethrow stored exception if any
}

template <class F, class Observer>
auto simcall_blocking(F&& code, Observer* observer) -> decltype(observer->get_result())
{
  simcall_blocking(std::forward<F>(code), static_cast<SimcallObserver*>(observer));
  return observer->get_result();
}
} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
