/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/cond.h>
#include <simgrid/s4u/ConditionVariable.hpp>
#include <xbt/log.h>

#include "simgrid/modelchecker.h"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_condition);

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
  xbt_assert(mutex != nullptr, "Cannot wait on a condition variable without a valid mutex");
  xbt_assert(mutex->get_owner() == issuer,
             "Cannot wait on a condvar with a mutex (id %u) owned by another actor (id %ld)", mutex->get_id(),
             mutex->get_owner()->get_pid());

  if (MC_is_active() || MC_record_replay_is_active()) { // Split in 2 simcalls for transition persistency

    kernel::actor::ConditionVariableObserver lock_observer{issuer, mc::Transition::Type::CONDVAR_ASYNC_LOCK, pimpl,
                                                           mutex, timeout};
    auto acquisition = kernel::actor::simcall_answered(
        [issuer, pimpl, mutex] { return pimpl->acquire_async(issuer, mutex); }, &lock_observer);

    kernel::actor::ConditionVariableObserver wait_observer{issuer, mc::Transition::Type::CONDVAR_WAIT,
                                                           acquisition.get(), timeout};
    bool res = kernel::actor::simcall_blocking(
        [issuer, &acquisition, timeout] { acquisition->wait_for(issuer, timeout); }, &wait_observer);

    /* get back the mutex after the wait */
    mutex->get_iface().lock();

    return res;

  } else { // Do it in one simcall only
    // We don't need no observer on this non-MC path, but simcall_blocking() requires it.
    // Use a type clearly indicating it's NO-MC in the hope to get a loud error if it gets used despite our
    // expectations.
    kernel::actor::ConditionVariableObserver observer{issuer, mc::Transition::Type::CONDVAR_NOMC, pimpl, mutex};

    return kernel::actor::simcall_blocking(
        [&observer, timeout] {
          auto issuer = observer.get_issuer();
          observer.get_cond()->acquire_async(issuer, observer.get_mutex())->wait_for(issuer, timeout);

          // The mutex_lock is done within wait_for
        },
        &observer);
  }
}

void ConditionVariable::wait(MutexPtr lock)
{
  do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock->pimpl_, -1);
}

void ConditionVariable::wait(const std::unique_lock<Mutex>& lock)
{
  do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock.mutex()->pimpl_, -1);
}

std::cv_status s4u::ConditionVariable::wait_for(MutexPtr lock, double timeout)
{
  // The simcall uses -1 for "any timeout" but we don't want this:
  if (timeout < 0)
    timeout = 0.0;

  bool timed_out = do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock->pimpl_, timeout);

  return timed_out ? std::cv_status::timeout : std::cv_status::no_timeout;
}
std::cv_status s4u::ConditionVariable::wait_for(const std::unique_lock<Mutex>& lock, double timeout)
{
  return wait_for(lock.mutex(), timeout);
}

std::cv_status ConditionVariable::wait_until(s4u::MutexPtr lock, double timeout_time)
{
  double now = Engine::get_clock();
  double timeout;
  if (timeout_time < now)
    timeout = 0.0;
  else
    timeout = timeout_time - now;

  bool timed_out = do_wait(kernel::actor::ActorImpl::self(), pimpl_, lock->pimpl_, timeout);

  return timed_out ? std::cv_status::timeout : std::cv_status::no_timeout;
}
std::cv_status ConditionVariable::wait_until(const std::unique_lock<Mutex>& lock, double timeout_time)
{
  return wait_until(lock.mutex(), timeout_time);
}

/**
 * Notify functions
 */
void ConditionVariable::notify_one()
{
  kernel::actor::ConditionVariableObserver observer{kernel::actor::ActorImpl::self(),
                                                    mc::Transition::Type::CONDVAR_SIGNAL, pimpl_};
  simgrid::kernel::actor::simcall_answered([this]() { pimpl_->signal(); }, &observer);
}

void ConditionVariable::notify_all()
{
  kernel::actor::ConditionVariableObserver observer{kernel::actor::ActorImpl::self(),
                                                    mc::Transition::Type::CONDVAR_BROADCAST, pimpl_};
  simgrid::kernel::actor::simcall_answered([this]() { pimpl_->broadcast(); }, &observer);
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
