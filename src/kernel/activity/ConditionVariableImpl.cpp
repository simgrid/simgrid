/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "simgrid/Exception.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_condition, simix_synchro, "Condition variables");

/********************************* Condition **********************************/

/** @brief Handle a condition waiting simcall without timeouts */
void simcall_HANDLER_cond_wait(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex)
{
  cond->wait(mutex, -1, simcall->issuer_);
}

/** @brief Handle a condition waiting simcall with timeouts */
void simcall_HANDLER_cond_wait_timeout(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex, double timeout)
{
  cond->wait(mutex, timeout, simcall->issuer_);
}

namespace simgrid {
namespace kernel {
namespace activity {

/**
 * @brief Signalizes a condition.
 *
 * Signalizes a condition and wakes up a sleeping process.
 * If there are no process sleeping, no action is done.
 */
void ConditionVariableImpl::signal()
{
  XBT_DEBUG("Signal condition %p", this);

  /* If there are processes waiting for the condition choose one and try
     to make it acquire the mutex */
  if (not sleeping_.empty()) {
    auto& proc = sleeping_.front();
    sleeping_.pop_front();

    /* Destroy waiter's synchronization */
    proc.waiting_synchro_ = nullptr;

    /* Now transform the cond wait simcall into a mutex lock one */
    smx_simcall_t simcall = &proc.simcall_;
    MutexImpl* simcall_mutex;
    if (simcall->call_ == SIMCALL_COND_WAIT)
      simcall_mutex = simcall_cond_wait__get__mutex(simcall);
    else
      simcall_mutex = simcall_cond_wait_timeout__get__mutex(simcall);
    simcall->call_ = SIMCALL_MUTEX_LOCK;

    simcall_mutex->lock(simcall->issuer_);
  }
  XBT_OUT();
}

/**
 * @brief Broadcasts a condition.
 *
 * Signal ALL processes waiting on a condition.
 * If there are no process waiting, no action is done.
 */
void ConditionVariableImpl::broadcast()
{
  XBT_DEBUG("Broadcast condition %p", this);

  /* Signal the condition until nobody is waiting on it */
  while (not sleeping_.empty())
    signal();
}

void ConditionVariableImpl::wait(smx_mutex_t mutex, double timeout, actor::ActorImpl* issuer)
{
  XBT_DEBUG("Wait condition %p", this);

  /* If there is a mutex unlock it */
  if (mutex != nullptr) {
    xbt_assert(mutex->get_owner() == issuer,
               "Actor %s cannot wait on ConditionVariable %p since it does not own the provided mutex %p",
               issuer->get_cname(), this, mutex);
    mutex_ = mutex;
    mutex->unlock(issuer);
  }

  RawImplPtr synchro(new RawImpl());
  synchro->set_host(issuer->get_host()).set_timeout(timeout).start();
  synchro->register_simcall(&issuer->simcall_);
  sleeping_.push_back(*issuer);
}

// boost::intrusive_ptr<ConditionVariableImpl> support:
void intrusive_ptr_add_ref(simgrid::kernel::activity::ConditionVariableImpl* cond)
{
  cond->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(simgrid::kernel::activity::ConditionVariableImpl* cond)
{
  if (cond->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    xbt_assert(cond->sleeping_.empty(), "Cannot destroy conditional since someone is still using it");
    delete cond;
  }
}
} // namespace activity
} // namespace kernel
} // namespace simgrid
