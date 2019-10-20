/* Copyright (c) 2007-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_HPP
#define SIMGRID_SIMIX_HPP

#include <simgrid/simix.h>
#include <xbt/functional.hpp>
#include <xbt/future.hpp>
#include <xbt/signal.hpp>

#include <boost/heap/fibonacci_heap.hpp>
#include <string>
#include <unordered_map>

XBT_PUBLIC void simcall_run_kernel(std::function<void()> const& code, simgrid::mc::SimcallInspector* t);
XBT_PUBLIC void simcall_run_blocking(std::function<void()> const& code, simgrid::mc::SimcallInspector* t);

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
template <class F> typename std::result_of<F()>::type simcall(F&& code, mc::SimcallInspector* t = nullptr)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall marshalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  typedef typename std::result_of<F()>::type R;
  simgrid::xbt::Result<R> result;
  simcall_run_kernel([&result, &code] { simgrid::xbt::fulfill_promise(result, std::forward<F>(code)); }, t);
  return result.get();
}

/** Execute some code (that does not return immediately) in kernel context
 *
 * This is very similar to simcall() right above, but the calling actor will not get rescheduled until
 * actor->simcall_answer() is called explicitly.
 *
 * Since the return value does not come from the lambda directly, its type cannot be guessed automatically and must
 * be provided as template parameter.
 *
 * This is meant for blocking actions. For example, locking a mutex is a blocking simcall.
 * First it's a simcall because that's obviously a modification of the world. Then, that's a blocking simcall because if
 * the mutex happens not to be free, the actor is added to a queue of actors in the mutex. Every mutex->unlock() takes
 * the first actor from the queue, mark it as current owner of the mutex and call actor->simcall_answer() to mark that
 * this mutex is now unblocked and ready to run again. If the mutex is initially free, the calling actor is unblocked
 * right away with actor->simcall_answer() once the mutex is marked as locked.
 *
 * If your code never calls actor->simcall_answer() itself, the actor will never return from its simcall.
 */
template <class R, class F> R simcall_blocking(F&& code, mc::SimcallInspector* t = nullptr)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall marshalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  simgrid::xbt::Result<R> result;
  simcall_run_blocking([&result, &code] { simgrid::xbt::fulfill_promise(result, std::forward<F>(code)); }, t);
  return result.get();
}
} // namespace actor
} // namespace kernel
} // namespace simgrid
namespace simgrid {
namespace simix {

// What's executed as SIMIX actor code:
typedef std::function<void()> ActorCode;

// Create an ActorCode based on a std::string
typedef std::function<ActorCode(std::vector<std::string> args)> ActorCodeFactory;

XBT_PUBLIC void register_function(const std::string& name, const ActorCodeFactory& factory);

typedef std::pair<double, Timer*> TimerQelt;
static boost::heap::fibonacci_heap<TimerQelt, boost::heap::compare<xbt::HeapComparator<TimerQelt>>> simix_timers;

/** @brief Timer datatype */
class Timer {
  double date = 0.0;

public:
  decltype(simix_timers)::handle_type handle_;

  Timer(double date, simgrid::xbt::Task<void()>&& callback) : date(date), callback(std::move(callback)) {}

  simgrid::xbt::Task<void()> callback;
  double get_date() { return date; }
  void remove();

  template <class F> static inline Timer* set(double date, F callback)
  {
    return set(date, simgrid::xbt::Task<void()>(std::move(callback)));
  }

  static Timer* set(double date, simgrid::xbt::Task<void()>&& callback);
  static double next() { return simix_timers.empty() ? -1.0 : simix_timers.top().first; }
};

} // namespace simix
} // namespace simgrid

XBT_PUBLIC smx_actor_t simcall_process_create(const std::string& name, const simgrid::simix::ActorCode& code,
                                              void* data, sg_host_t host,
                                              std::unordered_map<std::string, std::string>* properties);

#endif
