/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/cond.h"
#include "simgrid/forward.h"
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
      kernel::actor::simcall([] { return new kernel::activity::ConditionVariableImpl(); });
  return ConditionVariablePtr(cond->get_iface(), false);
}

/**
 * Wait functions
 */
void ConditionVariable::wait(MutexPtr lock)
{
  simcall_cond_wait(cond_, lock->pimpl_);
}

void ConditionVariable::wait(const std::unique_lock<Mutex>& lock)
{
  simcall_cond_wait(cond_, lock.mutex()->pimpl_);
}

std::cv_status s4u::ConditionVariable::wait_for(const std::unique_lock<Mutex>& lock, double timeout)
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

std::cv_status ConditionVariable::wait_until(const std::unique_lock<Mutex>& lock, double timeout_time)
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
  simgrid::kernel::actor::simcall([this]() { cond_->signal(); });
}

void ConditionVariable::notify_all()
{
  simgrid::kernel::actor::simcall([this]() { cond_->broadcast(); });
}

void intrusive_ptr_add_ref(const ConditionVariable* cond)
{
  intrusive_ptr_add_ref(cond->cond_);
}

void intrusive_ptr_release(const ConditionVariable* cond)
{
  intrusive_ptr_release(cond->cond_);
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
sg_cond_t sg_cond_init()
{
  simgrid::kernel::activity::ConditionVariableImpl* cond =
      simgrid::kernel::actor::simcall([] { return new simgrid::kernel::activity::ConditionVariableImpl(); });

  return new simgrid::s4u::ConditionVariable(cond);
}

void sg_cond_wait(sg_cond_t cond, sg_mutex_t mutex)
{
  cond->wait(mutex);
}

int sg_cond_wait_for(sg_cond_t cond, sg_mutex_t mutex, double delay)
{
  std::unique_lock<simgrid::s4u::Mutex> lock(*mutex);
  return cond->wait_for(lock, delay) == std::cv_status::timeout ? 1 : 0;
}

void sg_cond_notify_one(sg_cond_t cond)
{
  cond->notify_one();
}

void sg_cond_notify_all(sg_cond_t cond)
{
  cond->notify_all();
}

void sg_cond_destroy(const_sg_cond_t cond)
{
  delete cond;
}
