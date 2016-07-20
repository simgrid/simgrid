/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COND_VARIABLE_HPP
#define SIMGRID_S4U_COND_VARIABLE_HPP

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <utility> // std::swap

#include <boost/intrusive_ptr.hpp>

#include <xbt/base.h>

#include <simgrid/simix.h>
#include <simgrid/s4u/mutex.hpp>

namespace simgrid {
namespace s4u {

class Mutex;

/** A condition variable
 *
 *  This is based on std::condition_variable and should respect the same
 *  semantic. But we currently use (only) double for both durations and
 *  timestamp timeouts.
 */
XBT_PUBLIC_CLASS ConditionVariable {
private:
  friend s_smx_cond;
  smx_cond_t cond_;
  ConditionVariable(smx_cond_t cond) : cond_(cond) {}
public:

  ConditionVariable(ConditionVariable const&) = delete;
  ConditionVariable& operator=(ConditionVariable const&) = delete;

  friend XBT_PUBLIC(void) intrusive_ptr_add_ref(ConditionVariable* cond);
  friend XBT_PUBLIC(void) intrusive_ptr_release(ConditionVariable* cond);
  using Ptr = boost::intrusive_ptr<ConditionVariable>;

  static Ptr createConditionVariable();

  //  Wait functions:

  void wait(std::unique_lock<Mutex>& lock);
  std::cv_status wait_until(std::unique_lock<Mutex>& lock, double timeout_time);
  std::cv_status wait_for(std::unique_lock<Mutex>& lock, double duration);

  /** Wait for a given duraiton
   *
   *  This version gives us the ability to do (in C++):
   *
   *  <code>
   *  using namespace std::literals::chrono_literals;
   *
   *  cond->wait_for(lock, 1ms);
   *  cond->wait_for(lock, 1s);
   *  cond->wait_for(lock, 1min);
   *  cond->wait_for(lock, 1h);
   *  </code>
   */
  template<class Rep, class Period>
  std::cv_status wait_for(std::unique_lock<Mutex>& lock, std::chrono::duration<Rep, Period> duration)
  {
    typedef std::chrono::duration<double> SecondsDouble;
    auto seconds = std::chrono::duration_cast<SecondsDouble>(duration);
    return this->wait_for(lock, duration.count());
  }

  // Variants which takes a predicate:

  template<class P>
  void wait(std::unique_lock<Mutex>& lock, P pred)
  {
    while (!pred())
      wait(lock);
  }
  template<class P>
  bool wait_until(std::unique_lock<Mutex>& lock, double timeout_time, P pred)
  {
    while (!pred())
      if (this->wait_until(lock, timeout_time) == std::cv_status::timeout)
        return pred();
    return true;
  }
  template<class P>
  bool wait_for(std::unique_lock<Mutex>& lock, double duration, P pred)
  {
    return this->wait_until(lock, SIMIX_get_clock() + duration, std::move(pred));
  }
  template<class Rep, class Period, class P>
  bool wait_for(std::unique_lock<Mutex>& lock, std::chrono::duration<Rep, Period> duration, P pred)
  {
    typedef std::chrono::duration<double> SecondsDouble;
    auto seconds = std::chrono::duration_cast<SecondsDouble>(duration);
    return this->wait_for(lock, seconds.count(), pred);
  }

  // Notify functions

  void notify_one();
  void notify_all();

  XBT_ATTRIB_DEPRECATED("Use notify_one() instead")
  void notify() { notify_one(); }
};

using ConditionVariablePtr = ConditionVariable::Ptr;

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COND_VARIABLE_HPP */
