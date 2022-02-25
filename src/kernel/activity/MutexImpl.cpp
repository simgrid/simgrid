/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/Synchro.hpp"

#if SIMGRID_HAVE_MC
#include "simgrid/modelchecker.h"
#include "src/mc/mc_safety.hpp"
#define MC_CHECK_NO_DPOR()                                                                                             \
  xbt_assert(not MC_is_active() || mc::reduction_mode != mc::ReductionMode::dpor,                                      \
             "Mutex is currently not supported with DPOR,  use --cfg=model-check/reduction:none")
#else
#define MC_CHECK_NO_DPOR() (void)0
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_mutex, ker_synchro, "Mutex kernel-space implementation");

namespace simgrid {
namespace kernel {
namespace activity {

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
  smx_simcall_t simcall = simcalls_.front();
  simcalls_.pop_front();

  simcall->issuer_->waiting_synchro_ = nullptr;
  simcall->issuer_->simcall_answer();
}

unsigned MutexImpl::next_id_ = 0;

MutexAcquisitionImplPtr MutexImpl::lock_async(actor::ActorImpl* issuer)
{
  auto res = MutexAcquisitionImplPtr(new kernel::activity::MutexAcquisitionImpl(issuer, this), true);

  if (owner_ != nullptr) {
    /* Somebody is using the mutex; register the acquisition */
    sleeping_.push_back(res);
  } else {
    owner_  = issuer;
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
  XBT_IN("(%p, %p)", this, issuer);
  MC_CHECK_NO_DPOR();
  if (owner_ != nullptr) {
    XBT_OUT();
    return false;
  }

  owner_  = issuer;
  XBT_OUT();
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

  if (not sleeping_.empty()) {
    /* Give the ownership to the first waiting actor */
    auto acq = sleeping_.front();
    sleeping_.pop_front();

    owner_ = acq->get_issuer();
    if (acq == owner_->waiting_synchro_)
      acq->finish();
    // else, the issuer is not blocked on this acquisition so no need to release it

  } else {
    /* nobody to wake up */
    owner_  = nullptr;
  }
  XBT_OUT();
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
