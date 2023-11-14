/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/Synchro.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_mutex, ker_synchro, "Mutex kernel-space implementation");

namespace simgrid::kernel::activity {

/* -------- Acquisition -------- */

bool MutexAcquisitionImpl::test(actor::ActorImpl*)
{
  return mutex_->owner_ == issuer_;
}
void MutexAcquisitionImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  xbt_assert(mutex_->owner_ != nullptr); // it was locked either by someone else or by me during the lock_async
  xbt_assert(issuer == issuer_, "Cannot wait on acquisitions created by another actor (id %ld)", issuer_->get_pid());
  xbt_assert(timeout < 0, "Timeouts on mutex acquisitions are not implemented yet.");

  this->register_simcall(&issuer_->simcall_); // Block on that acquisition

  if (mutex_->get_owner() == issuer_) { // I'm the owner
    finish();
  } else {
    // Already in the queue
  }
}

void MutexAcquisitionImpl::finish()
{
  xbt_assert(simcalls_.size() == 1, "Unexpected number of simcalls waiting: %zu", simcalls_.size());
  actor::Simcall* simcall = simcalls_.front();
  simcalls_.pop_front();

  simcall->issuer_->waiting_synchro_ = nullptr;
  simcall->issuer_->simcall_answer();
}

/* -------- Mutex -------- */

unsigned MutexImpl::next_id_ = 0;

MutexAcquisitionImplPtr MutexImpl::lock_async(actor::ActorImpl* issuer)
{
  /* If the mutex is recursive */
  if (is_recursive_) {
    if (owner_ == issuer) {
      recursive_depth++;
      auto res = MutexAcquisitionImplPtr(new kernel::activity::MutexAcquisitionImpl(issuer, this), true);
      res->grant();
      return res;
    } else if (owner_ == nullptr) { // Free
      owner_          = issuer;
      recursive_depth = 1;
      auto res        = MutexAcquisitionImplPtr(new kernel::activity::MutexAcquisitionImpl(issuer, this), true);
      res->grant();
      return res;
    }

    for (auto acq : ongoing_acquisitions_)
      if (acq->get_issuer() == issuer) {
        acq->recursive_depth_++;
        return acq;
      }

    // Not yet in the ongoing acquisition list. Get in there
    auto res = MutexAcquisitionImplPtr(new kernel::activity::MutexAcquisitionImpl(issuer, this), true);
    ongoing_acquisitions_.push_back(res);
    return res;
  }

  // Non-recursive mutex
  auto res = MutexAcquisitionImplPtr(new kernel::activity::MutexAcquisitionImpl(issuer, this), true);
  if (owner_ == nullptr) { // Lock is free, take it
    owner_  = issuer;
    recursive_depth = 1;
    res->grant();
  } else { // Somebody is using the mutex; register the acquisition
    ongoing_acquisitions_.push_back(res);
  }
  return res;
}

/** Tries to lock the mutex for a actor
 *
 * @param  issuer  the actor that tries to acquire the mutex
 * @return whether we managed to lock the mutex
 */
bool MutexImpl::try_lock(actor::ActorImpl* issuer)
{
  if (owner_ == issuer && is_recursive_) {
    recursive_depth++;
    return true;
  }
  if (owner_ != nullptr)
    return false;

  owner_ = issuer;
  return true;
}

/** Unlock a mutex for a actor
 *
 * Unlocks the mutex and gives it to a actor waiting for it.
 * If the unlocker is not the owner of the mutex nothing happens.
 * If there are no actor waiting, it sets the mutex as free.
 */
void MutexImpl::unlock(actor::ActorImpl* issuer)
{
  XBT_IN("(%p, %p)", this, issuer);
  xbt_assert(issuer == owner_, "Cannot release that mutex: you're not the owner. %s is (pid:%ld).",
             owner_ != nullptr ? owner_->get_cname() : "(nobody)", owner_ != nullptr ? owner_->get_pid() : -1);

  if (is_recursive_) {
    recursive_depth--;
    if (recursive_depth > 0) // Still owning the lock
      return;
  }

  if (not ongoing_acquisitions_.empty()) {
    /* Give the ownership to the first waiting actor */
    auto acq = ongoing_acquisitions_.front();
    ongoing_acquisitions_.pop_front();

    owner_ = acq->get_issuer();
    acq->grant();
    recursive_depth = acq->recursive_depth_;
    if (acq == owner_->waiting_synchro_)
      acq->finish();
    // else, the issuer is not blocked on this acquisition so no need to release it

  } else {
    /* nobody to wake up */
    owner_  = nullptr;
  }
  XBT_OUT();
}

} // namespace simgrid::kernel::activity
