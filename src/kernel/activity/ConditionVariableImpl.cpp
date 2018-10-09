/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "simgrid/Exception.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/simix/smx_synchro_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ConditionVariable, simix_synchro, "Condition variables");

/********************************* Condition **********************************/

static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout, smx_actor_t issuer,
                             smx_simcall_t simcall)
{
  XBT_IN("(%p, %p, %f, %p,%p)", cond, mutex, timeout, issuer, simcall);
  smx_activity_t synchro = nullptr;

  XBT_DEBUG("Wait condition %p", cond);

  /* If there is a mutex unlock it */
  /* FIXME: what happens if the issuer is not the owner of the mutex? */
  if (mutex != nullptr) {
    cond->mutex = mutex;
    mutex->unlock(issuer);
  }

  synchro = SIMIX_synchro_wait(issuer->host_, timeout);
  synchro->simcalls_.push_front(simcall);
  issuer->waiting_synchro = synchro;
  cond->sleeping.push_back(*simcall->issuer);
  XBT_OUT();
}

/**
 * @brief Handle a condition waiting simcall without timeouts
 */
void simcall_HANDLER_cond_wait(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex)
{
  XBT_IN("(%p)", simcall);
  smx_actor_t issuer = simcall->issuer;

  _SIMIX_cond_wait(cond, mutex, -1, issuer, simcall);
  XBT_OUT();
}

/**
 * @brief Handle a condition waiting simcall with timeouts
 */
void simcall_HANDLER_cond_wait_timeout(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex, double timeout)
{
  XBT_IN("(%p)", simcall);
  smx_actor_t issuer = simcall->issuer;
  simcall_cond_wait_timeout__set__result(simcall, 0); // default result, will be set to 1 on timeout
  _SIMIX_cond_wait(cond, mutex, timeout, issuer, simcall);
  XBT_OUT();
}

namespace simgrid {
namespace kernel {
namespace activity {

ConditionVariableImpl::ConditionVariableImpl() : cond_(this) {}
ConditionVariableImpl::~ConditionVariableImpl() = default;

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
  if (not sleeping.empty()) {
    auto& proc = sleeping.front();
    sleeping.pop_front();

    /* Destroy waiter's synchronization */
    proc.waiting_synchro = nullptr;

    /* Now transform the cond wait simcall into a mutex lock one */
    smx_simcall_t simcall = &proc.simcall;
    smx_mutex_t simcall_mutex;
    if (simcall->call == SIMCALL_COND_WAIT)
      simcall_mutex = simcall_cond_wait__get__mutex(simcall);
    else
      simcall_mutex = simcall_cond_wait_timeout__get__mutex(simcall);
    simcall->call = SIMCALL_MUTEX_LOCK;

    simcall_HANDLER_mutex_lock(simcall, simcall_mutex);
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
  while (not sleeping.empty())
    signal();
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
    xbt_assert(cond->sleeping.empty(), "Cannot destroy conditional since someone is still using it");
    delete cond;
  }
}
} // namespace activity
} // namespace kernel
}
