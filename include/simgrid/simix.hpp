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

XBT_PUBLIC void simcall_run_kernel(std::function<void()> const& code);

/** Execute some code in the kernel and block
 *
 * run_blocking() is a generic blocking simcall. It is given a callback
 * which is executed immediately in the SimGrid kernel. The callback is
 * responsible for setting the suitable logic for waking up the process
 * when needed.
 *
 * @ref simix::kernelSync() is a higher level wrapper for this.
 */
XBT_PUBLIC void simcall_run_blocking(std::function<void()> const& code);

template<class F> inline
void simcall_run_kernel(F& f)
{
  simcall_run_kernel(std::function<void()>(std::ref(f)));
}
template<class F> inline
void simcall_run_blocking(F& f)
{
  simcall_run_blocking(std::function<void()>(std::ref(f)));
}

namespace simgrid {
namespace simix {

/** Execute some code in the kernel/maestro
 *
 *  This can be used to enforce mutual exclusion with other simcall.
 *  More importantly, this enforces a deterministic/reproducible ordering
 *  of the operation with respect to other simcalls.
 */
template <class F> typename std::result_of<F()>::type simcall(F&& code)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall mashalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  typedef typename std::result_of<F()>::type R;
  simgrid::xbt::Result<R> result;
  simcall_run_kernel([&result, &code] { simgrid::xbt::fulfill_promise(result, std::forward<F>(code)); });
  return result.get();
}

XBT_ATTRIB_DEPRECATED_v325("Please manifest if you actually need this function")
    XBT_PUBLIC const std::vector<smx_actor_t>& process_get_runnable();

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

  template <class R, class T>
  XBT_ATTRIB_DEPRECATED_v325("Please use a lambda or std::bind") static inline Timer* set(double date,
                                                                                          R (*callback)(T*), T* arg)
  {
    return set(date, std::bind(callback, arg));
  }

  XBT_ATTRIB_DEPRECATED_v325("Please use a lambda or std::bind") static Timer* set(double date, void (*callback)(void*),
                                                                                   void* arg)
  {
    return set(date, std::bind(callback, arg));
  }
  static Timer* set(double date, simgrid::xbt::Task<void()>&& callback);
  static double next() { return simix_timers.empty() ? -1.0 : simix_timers.top().first; }
};

} // namespace simix
} // namespace simgrid

XBT_PUBLIC smx_actor_t simcall_process_create(const std::string& name, const simgrid::simix::ActorCode& code,
                                              void* data, sg_host_t host,
                                              std::unordered_map<std::string, std::string>* properties);

XBT_ATTRIB_DEPRECATED_v325("Please use simgrid::xbt::Timer::set") XBT_PUBLIC smx_timer_t
    SIMIX_timer_set(double date, simgrid::xbt::Task<void()>&& callback);

#endif
