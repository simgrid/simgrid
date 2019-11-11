/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COND_VARIABLE_HPP
#define SIMGRID_S4U_COND_VARIABLE_HPP

#include <simgrid/forward.h>

#include <simgrid/chrono.hpp>
#include <simgrid/s4u/Mutex.hpp>

#include <future>

namespace simgrid {
namespace s4u {

/** @brief A condition variable
 *  @ingroup s4u_api
 *
 *  This is a drop-in replacement of `std::condition_variable` and should respect the same
 *  semantic. But we currently use (only) double for both durations and
 *  timestamp timeouts.
 */
class XBT_PUBLIC ConditionVariable {
private:
  friend kernel::activity::ConditionVariableImpl;
  kernel::activity::ConditionVariableImpl* const cond_;

public:
  explicit ConditionVariable(kernel::activity::ConditionVariableImpl* cond) : cond_(cond) {}

#ifndef DOXYGEN
  ConditionVariable(ConditionVariable const&) = delete;
  ConditionVariable& operator=(ConditionVariable const&) = delete;

  friend XBT_PUBLIC void intrusive_ptr_add_ref(ConditionVariable * cond);
  friend XBT_PUBLIC void intrusive_ptr_release(ConditionVariable * cond);
#endif

  static ConditionVariablePtr create();

  //  Wait functions without time:

  void wait(MutexPtr lock);
  void wait(std::unique_lock<Mutex> & lock);
  template <class P> void wait(std::unique_lock<Mutex> & lock, P pred)
  {
    while (not pred())
      wait(lock);
  }

  // Wait function taking a plain double as time:

  std::cv_status wait_until(std::unique_lock<Mutex> & lock, double timeout_time);
  std::cv_status wait_for(std::unique_lock<Mutex> & lock, double duration);
  template <class P> bool wait_until(std::unique_lock<Mutex> & lock, double timeout_time, P pred)
  {
    while (not pred())
      if (this->wait_until(lock, timeout_time) == std::cv_status::timeout)
        return pred();
    return true;
  }
  template <class P> bool wait_for(std::unique_lock<Mutex> & lock, double duration, P pred)
  {
    return this->wait_until(lock, SIMIX_get_clock() + duration, std::move(pred));
  }

  // Wait function taking a C++ style time:

  template <class Rep, class Period, class P>
  bool wait_for(std::unique_lock<Mutex> & lock, std::chrono::duration<Rep, Period> duration, P pred)
  {
    auto seconds = std::chrono::duration_cast<SimulationClockDuration>(duration);
    return this->wait_for(lock, seconds.count(), pred);
  }
  template <class Rep, class Period>
  std::cv_status wait_for(std::unique_lock<Mutex> & lock, std::chrono::duration<Rep, Period> duration)
  {
    auto seconds = std::chrono::duration_cast<SimulationClockDuration>(duration);
    return this->wait_for(lock, seconds.count());
  }
  template <class Duration>
  std::cv_status wait_until(std::unique_lock<Mutex> & lock, const SimulationTimePoint<Duration>& timeout_time)
  {
    auto timeout_native = std::chrono::time_point_cast<SimulationClockDuration>(timeout_time);
    return this->wait_until(lock, timeout_native.time_since_epoch().count());
  }
  template <class Duration, class P>
  bool wait_until(std::unique_lock<Mutex> & lock, const SimulationTimePoint<Duration>& timeout_time, P pred)
  {
    auto timeout_native = std::chrono::time_point_cast<SimulationClockDuration>(timeout_time);
    return this->wait_until(lock, timeout_native.time_since_epoch().count(), std::move(pred));
  }

  // Notify functions

  void notify_one();
  void notify_all();
};

} // namespace s4u
} // namespace simgrid

#endif
