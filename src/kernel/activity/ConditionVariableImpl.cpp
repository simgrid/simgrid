/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"
#include <cmath> // std::isfinite

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_condition, ker_synchro, "Condition variables kernel-space implementation");

/********************************* Condition **********************************/

namespace simgrid::kernel::activity {

/**
 * @brief Signalizes a condition.
 *
 * Signalizes a condition and wakes up a sleeping actor.
 * If there are no actor sleeping, no action is done.
 */
void ConditionVariableImpl::signal()
{
  XBT_DEBUG("Signal condition %p", this);

  /* If there are actors waiting for the condition choose one and try to make it acquire the mutex */
  if (not sleeping_.empty()) {
    auto& proc = sleeping_.front();
    sleeping_.pop_front();

    /* Destroy waiter's synchronization */
    // There is no CondVarAcquisition yet, so the only activity that could be there is probably a TimeoutDetector
    // Be brutal for now, it will get cleaned and unified with the rest of SimGrid when adding Acquisitions.
    xbt_assert(proc.waiting_synchros_.size() <= 1, 
               "Unexpected amount of activities in this actor: %zu. Are you using condvar in waitany?",
               proc.waiting_synchros_.size());
    proc.waiting_synchros_.clear();
    if (proc.simcall_.timeout_cb_) {
      proc.simcall_.timeout_cb_->remove();
      proc.simcall_.timeout_cb_ = nullptr;
    }

    /* Now transform the cond wait simcall into a mutex lock one */
    actor::Simcall* simcall = &proc.simcall_;
    const auto* observer    = dynamic_cast<kernel::actor::ConditionVariableObserver*>(simcall->observer_);
    xbt_assert(observer != nullptr);
    observer->get_mutex()->lock_async(simcall->issuer_)->wait_for(simcall->issuer_, -1);
  }
  XBT_OUT();
}

/**
 * @brief Broadcasts a condition.
 *
 * Signal ALL actors waiting on a condition.
 * If there are no actor waiting, no action is done.
 */
void ConditionVariableImpl::broadcast()
{
  XBT_DEBUG("Broadcast condition %p", this);

  /* Signal the condition until nobody is waiting on it */
  while (not sleeping_.empty())
    signal();
}

void ConditionVariableImpl::wait(MutexImpl* mutex, double timeout, actor::ActorImpl* issuer)
{
  XBT_DEBUG("Wait condition %p", this);
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  /* Unlock the provided mutex (the simcall observer ensures that one is provided, no need to check) */
  auto* owner = mutex->get_owner();
  xbt_assert(owner == issuer,
             "Actor %s cannot wait on ConditionVariable %p since it does not own the provided mutex %p (which is "
             "owned by %s).",
             issuer->get_cname(), this, mutex, (owner == nullptr ? "nobody" : owner->get_cname()));
  mutex_ = mutex;
  mutex->unlock(issuer);

  sleeping_.push_back(*issuer);
  if (timeout >= 0) {
    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [this, issuer]() {
      issuer->simcall_.timeout_cb_ = nullptr;
      this->remove_sleeping_actor(*issuer);
      auto* observer = dynamic_cast<kernel::actor::ConditionVariableObserver*>(issuer->simcall_.observer_);
      xbt_assert(observer != nullptr);
      observer->set_result(true);

      issuer->simcall_answer();
    });
  }
}

// boost::intrusive_ptr<ConditionVariableImpl> support:
void intrusive_ptr_add_ref(ConditionVariableImpl* cond)
{
  cond->refcount_.fetch_add(1, std::memory_order_relaxed);
}

void intrusive_ptr_release(ConditionVariableImpl* cond)
{
  if (cond->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    xbt_assert(cond->sleeping_.empty(), "Cannot destroy conditional since someone is still using it");
    delete cond;
  }
}
} // namespace simgrid::kernel::activity
