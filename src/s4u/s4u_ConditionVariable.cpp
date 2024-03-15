/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/cond.h>
#include <simgrid/s4u/ConditionVariable.hpp>
#include <xbt/log.h>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"

#include <mutex>

namespace simgrid::s4u {

ConditionVariablePtr ConditionVariable::create()
{
  kernel::activity::ConditionVariableImpl* cond =
      kernel::actor::simcall_answered([] { return new kernel::activity::ConditionVariableImpl(); });
  return ConditionVariablePtr(cond->get_iface(), false);
}

/**
 * Wait functions
 */
static bool do_wait(kernel::actor::ActorImpl* issuer, kernel::activity::ConditionVariableImpl* pimpl,
                    kernel::activity::MutexImpl* mutex, double timeout)
{
  kernel::actor::ConditionVariableObserver observer{issuer, pimpl, mutex};
  return kernel::actor::simcall_blocking(
      [&observer, timeout] {
        observer.get_cond()
            ->acquire_async(observer.get_issuer(), observer.get_mutex())
            ->wait_for(observer.get_issuer(), timeout);
      },
      &observer);
}

void ConditionVariable::wait(MutexPtr lock)
{
  do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock->pimpl_, -1);
}

void ConditionVariable::wait(const std::unique_lock<Mutex>& lock)
{
  do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock.mutex()->pimpl_, -1);
}

std::cv_status s4u::ConditionVariable::wait_for(const std::unique_lock<Mutex>& lock, double timeout)
{
  // The simcall uses -1 for "any timeout" but we don't want this:
  if (timeout < 0)
    timeout = 0.0;

  bool timed_out = do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock.mutex()->pimpl_, timeout);

  return timed_out ? std::cv_status::timeout : std::cv_status::no_timeout;
}

std::cv_status ConditionVariable::wait_until(const std::unique_lock<Mutex>& lock, double timeout_time)
{
  double now = Engine::get_clock();
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
  simgrid::kernel::actor::simcall_answered([this]() { pimpl_->signal(); });
}

void ConditionVariable::notify_all()
{
  simgrid::kernel::actor::simcall_answered([this]() { pimpl_->broadcast(); });
}

void intrusive_ptr_add_ref(const ConditionVariable* cond)
{
  intrusive_ptr_add_ref(cond->pimpl_);
}

void intrusive_ptr_release(const ConditionVariable* cond)
{
  intrusive_ptr_release(cond->pimpl_);
}

} // namespace simgrid::s4u

/* **************************** Public C interface *************************** */
sg_cond_t sg_cond_init()
{
  return simgrid::s4u::ConditionVariable::create().detach();
}

void sg_cond_wait(sg_cond_t cond, sg_mutex_t mutex)
{
  cond->wait(mutex);
}

int sg_cond_wait_for(sg_cond_t cond, sg_mutex_t mutex, double delay)
{
  std::unique_lock lock(*mutex);
  return cond->wait_for(lock, delay) == std::cv_status::timeout;
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
  intrusive_ptr_release(cond);
}
