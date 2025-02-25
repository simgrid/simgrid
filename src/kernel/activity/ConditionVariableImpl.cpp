/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "simgrid/modelchecker.h"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include <cmath> // std::isfinite

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_condition, ker_synchro, "Condition variables kernel-space implementation");

namespace simgrid::kernel::activity {

/* -------- Acquisition -------- */
ConditionVariableAcquisitionImpl::ConditionVariableAcquisitionImpl(actor::ActorImpl* issuer,
                                                                   ConditionVariableImpl* cond, MutexImpl* mutex)
    : issuer_(issuer), mutex_(mutex), cond_(cond)
{
  set_name(std::string("on condvar ") + std::to_string(cond_->get_id()));
}

void ConditionVariableAcquisitionImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  xbt_assert(issuer == issuer_, "Cannot wait on acquisitions created by another actor (id %ld)", issuer_->get_pid());

  XBT_DEBUG("Wait condition variable %u (timeout:%f)", cond_->get_id(), timeout);

  this->register_simcall(&issuer_->simcall_); // Block on that acquisition

  if (granted_) {
    finish();
  } else if (timeout > 0) {
    if (MC_is_active() || MC_record_replay_is_active()) {
      mc_timeout_ = true;
      finish(); // If there is a timeout and the acquisition is not OK, then let's timeout
    } else {
      model_action_ = get_issuer()->get_host()->get_cpu()->sleep(timeout);
      model_action_->set_activity(this);
    }
  }
  // Already in the queue
}
void ConditionVariableAcquisitionImpl::finish()
{
  auto* observer = dynamic_cast<kernel::actor::ConditionVariableObserver*>(get_issuer()->simcall_.observer_);
  xbt_assert(observer != nullptr);

  if (model_action_ != nullptr) {                                          // A timeout was declared
    if (model_action_->get_state() == resource::Action::State::FINISHED) { // The timeout elapsed
      if (granted_) {                                                      // but we got signaled, just in time!
        set_state(State::DONE);

      } else {    // we have to report that timeout
        cancel(); // Unregister the acquisition from the condvar

        /* Return to the englobing simcall that the wait_for timeouted */
        observer->set_result(true);
      }
    }
    model_action_->unref();
    model_action_ = nullptr;
  }
  if (mc_timeout_) { // A timeout was declared in MC mode, and it cannot be granted right away
    cancel();        // Unregister the acquisition from the condvar
    observer->set_result(true);
  }

  xbt_assert(simcalls_.size() == 1, "Unexpected number of simcalls waiting: %zu", simcalls_.size());
  auto issuer = unregister_first_simcall();

  /* Break the simcall in MC mode, as the lock is done in another simcall */
  if (observer->get_type() != mc::Transition::Type::CONDVAR_NOMC) {
    issuer->simcall_answer();
    return;
  }

  if (issuer == nullptr) /* don't answer exiting and dying actors */
    return;

  /* Now transform the cond wait simcall into a mutex lock one */
  actor::Simcall* simcall = &issuer->simcall_;
  observer->get_mutex()->lock_async(simcall->issuer_)->wait_for(simcall->issuer_, -1);
}
void ConditionVariableAcquisitionImpl::cancel()
{
  /* Remove myself from the list of interested parties */
  const auto* issuer = get_issuer();
  auto it            = std::find_if(cond_->ongoing_acquisitions_.begin(), cond_->ongoing_acquisitions_.end(),
                                    [issuer](ConditionVariableAcquisitionImplPtr acqui) { return acqui->get_issuer() == issuer; });
  xbt_assert(it != cond_->ongoing_acquisitions_.end(), "Cannot find myself in the waiting queue that I have to leave");
  cond_->ongoing_acquisitions_.erase(it);
  get_issuer()->activities_.erase(this);
}

/* -------- Condition -------- */
unsigned ConditionVariableImpl::next_id_ = 0;

/**
 * @brief Signals a condition.
 *
 * Signals a condition and wakes up a sleeping actor.
 * If there are no actor sleeping, no action is done.
 */
void ConditionVariableImpl::signal()
{
  XBT_DEBUG("Signal condition #%u", this->id_);
  if (not ongoing_acquisitions_.empty()) {

    /* Release the first waiting actor */
    auto acqui = ongoing_acquisitions_.front();
    ongoing_acquisitions_.pop_front();

    acqui->granted_ = true;

    // Finish the acquisition if the owner is already blocked on its completion
    auto& synchros = acqui->get_issuer()->waiting_synchros_;
    if (std::find(synchros.begin(), synchros.end(), acqui) != synchros.end())
      acqui->finish();
    // else, the issuer is not blocked on this acquisition so no need to release it
  }
  // else, nobody is waiting on the condition. The signal is lost.
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
  while (not ongoing_acquisitions_.empty())
    signal();
}

ConditionVariableAcquisitionImplPtr ConditionVariableImpl::acquire_async(actor::ActorImpl* issuer, MutexImpl* mutex)
{
  XBT_DEBUG("Wait_async condition #%u", this->get_id());

  /* Unlock the provided mutex (the simcall observer ensures that one mutex is provided, no need to check) */
  const auto* owner = mutex->get_owner();
  xbt_assert(owner == issuer,
             "Actor %s cannot wait on ConditionVariable #%u since it does not own the provided mutex #%u (which is "
             "owned by %s).",
             issuer->get_cname(), this->get_id(), mutex->get_id(), (owner == nullptr ? "nobody" : owner->get_cname()));
  mutex->unlock(issuer);

  auto res = ConditionVariableAcquisitionImplPtr(new ConditionVariableAcquisitionImpl(issuer, this, mutex), true);
  // A condvar::wait is always blocking, it can never be created in a granted state (to the contrary to mutex or
  // semaphore)
  ongoing_acquisitions_.push_back(res);
  return res;
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
    xbt_assert(cond->ongoing_acquisitions_.empty(),
               "Cannot destroy conditional since it still has ongoing acquisitions");
    delete cond;
  }
}
} // namespace simgrid::kernel::activity
