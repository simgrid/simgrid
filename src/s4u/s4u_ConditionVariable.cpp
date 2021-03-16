/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/cond.h"
#include "simgrid/forward.h"
#include "simgrid/s4u/ConditionVariable.hpp"
#include "simgrid/simix.h"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/mc/checker/SimcallObserver.hpp"
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
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  mc::ConditionWaitSimcall observer{issuer, pimpl_, lock->pimpl_};
  kernel::actor::simcall_blocking<void>(
      [&observer] { observer.get_cond()->wait(observer.get_mutex(), -1.0, observer.get_issuer()); }, &observer);
}

void ConditionVariable::wait(const std::unique_lock<Mutex>& lock)
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  mc::ConditionWaitSimcall observer{issuer, pimpl_, lock.mutex()->pimpl_};
  kernel::actor::simcall_blocking<void>(
      [&observer] { observer.get_cond()->wait(observer.get_mutex(), -1.0, observer.get_issuer()); }, &observer);
}

std::cv_status s4u::ConditionVariable::wait_for(const std::unique_lock<Mutex>& lock, double timeout)
{
  // The simcall uses -1 for "any timeout" but we don't want this:
  if (timeout < 0)
    timeout = 0.0;

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  mc::ConditionWaitSimcall observer{issuer, pimpl_, lock.mutex()->pimpl_, timeout};
  kernel::actor::simcall_blocking<void>(
      [&observer] { observer.get_cond()->wait(observer.get_mutex(), observer.get_timeout(), observer.get_issuer()); },
      &observer);
  bool timed_out = simgrid::simix::unmarshal<bool>(issuer->simcall_.result_);
  if (timed_out) {
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
  simgrid::kernel::actor::simcall([this]() { pimpl_->signal(); });
}

void ConditionVariable::notify_all()
{
  simgrid::kernel::actor::simcall([this]() { pimpl_->broadcast(); });
}

void intrusive_ptr_add_ref(const ConditionVariable* cond)
{
  intrusive_ptr_add_ref(cond->pimpl_);
}

void intrusive_ptr_release(const ConditionVariable* cond)
{
  intrusive_ptr_release(cond->pimpl_);
}

} // namespace s4u
} // namespace simgrid

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
  std::unique_lock<simgrid::s4u::Mutex> lock(*mutex);
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
