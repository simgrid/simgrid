/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>
#include <mutex>

#include <xbt/ex.hpp>
#include <xbt/log.hpp>

#include "simgrid/s4u/ConditionVariable.hpp"
#include "simgrid/simix.h"
#include "src/simix/smx_synchro_private.hpp"

namespace simgrid {
namespace s4u {

ConditionVariablePtr ConditionVariable::createConditionVariable()
{
  smx_cond_t cond = simcall_cond_init();
  return ConditionVariablePtr(&cond->cond_, false);
}

/**
 * Wait functions
 */
void ConditionVariable::wait(MutexPtr lock)
{
  simcall_cond_wait(cond_, lock->mutex_);
}

void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  simcall_cond_wait(cond_, lock.mutex()->mutex_);
}

std::cv_status s4u::ConditionVariable::wait_for(std::unique_lock<Mutex>& lock, double timeout) {
  // The simcall uses -1 for "any timeout" but we don't want this:
  if (timeout < 0)
    timeout = 0.0;

  try {
    simcall_cond_wait_timeout(cond_, lock.mutex()->mutex_, timeout);
    return std::cv_status::no_timeout;
  }
  catch (xbt_ex& e) {

    // If the exception was a timeout, we have to take the lock again:
    if (e.category == timeout_error) {
      try {
        lock.mutex()->lock();
        return std::cv_status::timeout;
      }
      catch (...) {
        std::terminate();
      }
    }

    // Another exception: should we reaquire the lock?
    std::terminate();
  }
  catch (...) {
    std::terminate();
  }
}

std::cv_status ConditionVariable::wait_until(std::unique_lock<Mutex>& lock, double timeout_time)
{
  double now = SIMIX_get_clock();
  double timeout;
  if (timeout_time < now)
    timeout = 0.0;
  else
    timeout = timeout_time - now;
  return this->wait_for(lock, timeout);
}

/**
 * Notify functions
 */
void ConditionVariable::notify_one() {
   simcall_cond_signal(cond_);
}

void ConditionVariable::notify_all() {
  simcall_cond_broadcast(cond_);
}

void intrusive_ptr_add_ref(ConditionVariable* cond)
{
  intrusive_ptr_add_ref(cond->cond_);
}

void intrusive_ptr_release(ConditionVariable* cond)
{
  intrusive_ptr_release(cond->cond_);
}

}
}
