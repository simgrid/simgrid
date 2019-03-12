/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/ConditionVariable.hpp"
#include "simgrid/simix.h"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "xbt/log.hpp"

#include <exception>
#include <mutex>

namespace simgrid {
namespace s4u {

ConditionVariablePtr ConditionVariable::create()
{
  kernel::activity::ConditionVariableImpl* cond =
      simix::simcall([] { return new kernel::activity::ConditionVariableImpl(); });
  return ConditionVariablePtr(&cond->cond_, false);
}

/**
 * Wait functions
 */
void ConditionVariable::wait(MutexPtr lock)
{
  simcall_cond_wait(cond_, lock->pimpl_);
}

void ConditionVariable::wait(std::unique_lock<Mutex>& lock)
{
  simcall_cond_wait(cond_, lock.mutex()->pimpl_);
}

std::cv_status s4u::ConditionVariable::wait_for(std::unique_lock<Mutex>& lock, double timeout)
{
  // The simcall uses -1 for "any timeout" but we don't want this:
  if (timeout < 0)
    timeout = 0.0;

  if (simcall_cond_wait_timeout(cond_, lock.mutex()->pimpl_, timeout)) {
    // If we reached the timeout, we have to take the lock again:
    lock.mutex()->lock();
    return std::cv_status::timeout;
  } else {
    return std::cv_status::no_timeout;
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
void ConditionVariable::notify_one()
{
  simgrid::simix::simcall([this]() { cond_->signal(); });
}

void ConditionVariable::notify_all()
{
  simgrid::simix::simcall([this]() { cond_->broadcast(); });
}

void intrusive_ptr_add_ref(ConditionVariable* cond)
{
  intrusive_ptr_add_ref(cond->cond_);
}

void intrusive_ptr_release(ConditionVariable* cond)
{
  intrusive_ptr_release(cond->cond_);
}

} // namespace s4u
} // namespace simgrid
