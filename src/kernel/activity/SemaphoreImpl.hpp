/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP

#include <atomic>
#include <boost/intrusive/list.hpp>

#include "simgrid/s4u/Semaphore.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

/** Semaphore Acquisition: the act / process of acquiring the semaphore.
 *
 * You can declare some interest on a semaphore without being blocked waiting if it's already empty.
 * See the documentation of the MutexAcquisitionImpl for further details.
 */
class XBT_PUBLIC SemAcquisitionImpl : public ActivityImpl_T<SemAcquisitionImpl> {
  actor::ActorImpl* issuer_ = nullptr;
  SemaphoreImpl* semaphore_ = nullptr;
  bool granted_             = false;

  friend SemaphoreImpl;
  friend actor::SemaphoreAcquisitionObserver;

public:
  SemAcquisitionImpl(actor::ActorImpl* issuer, SemaphoreImpl* sem) : issuer_(issuer), semaphore_(sem) {}
  SemaphoreImplPtr get_semaphore() { return semaphore_; }
  actor::ActorImpl* get_issuer() { return issuer_; }

  bool test(actor::ActorImpl* issuer = nullptr) override { return granted_; }
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  void post() override;
  void finish() override;
  void cancel() override;
  void set_exception(actor::ActorImpl* issuer) override
  { /* nothing to do */
  }
};

class XBT_PUBLIC SemaphoreImpl {
  std::atomic_int_fast32_t refcount_{1};
  s4u::Semaphore piface_;
  unsigned int value_;
  std::deque<SemAcquisitionImplPtr> ongoing_acquisitions_;

  static unsigned next_id_;
  unsigned id_ = next_id_++;

  friend SemAcquisitionImpl;
  friend actor::SemaphoreObserver;

public:
  explicit SemaphoreImpl(unsigned int value) : piface_(this), value_(value){};

  SemaphoreImpl(SemaphoreImpl const&) = delete;
  SemaphoreImpl& operator=(SemaphoreImpl const&) = delete;

  SemAcquisitionImplPtr acquire_async(actor::ActorImpl* issuer);
  void release();
  bool would_block() const { return (value_ == 0); }

  unsigned int get_capacity() const { return value_; }
  bool is_used() const { return not ongoing_acquisitions_.empty(); }

  friend void intrusive_ptr_add_ref(SemaphoreImpl* sem)
  {
    XBT_ATTRIB_UNUSED auto previous = sem->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }
  friend void intrusive_ptr_release(SemaphoreImpl* sem)
  {
    if (sem->refcount_.fetch_sub(1) == 1) {
      xbt_assert(not sem->is_used(), "Cannot destroy semaphore since someone is still using it");
      delete sem;
    }
  }
  unsigned get_id() { return id_; }

  s4u::Semaphore& sem() { return piface_; }
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ACTIVITY_SEMAPHOREIMPL_HPP */
