/* Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_BARRIER_HPP
#define SIMGRID_KERNEL_ACTIVITY_BARRIER_HPP

#include "simgrid/s4u/Barrier.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SynchroObserver.hpp"

namespace simgrid {
namespace kernel {
namespace activity {
/** Barrier Acquisition: the act / process of acquiring the barrier.
 *
 * This is the asynchronous activity associated to Barriers. See the doc of MutexImpl for more details on the rationnal.
 */
class XBT_PUBLIC BarrierAcquisitionImpl : public ActivityImpl_T<BarrierAcquisitionImpl> {
  actor::ActorImpl* issuer_ = nullptr;
  BarrierImpl* barrier_     = nullptr;
  bool granted_             = false;

  friend actor::BarrierObserver;
  friend BarrierImpl;

public:
  BarrierAcquisitionImpl(actor::ActorImpl* issuer, BarrierImpl* bar) : issuer_(issuer), barrier_(bar) {}
  BarrierImplPtr get_barrier() { return barrier_; }
  actor::ActorImpl* get_issuer() { return issuer_; }

  bool test(actor::ActorImpl* issuer = nullptr) override;
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  void post() override
  { /*no surf action*/
  }
  void finish() override;
  void set_exception(actor::ActorImpl* issuer) override
  { /* nothing to do */
  }
};

class XBT_PUBLIC BarrierImpl {
  std::atomic_int_fast32_t refcount_{1};
  s4u::Barrier piface_;
  unsigned int expected_actors_;
  std::deque<BarrierAcquisitionImplPtr> ongoing_acquisitions_;
  static unsigned next_id_;
  unsigned id_ = next_id_++;

  friend BarrierAcquisitionImpl;
  friend s4u::Barrier;

public:
  explicit BarrierImpl(int expected_actors) : piface_(this), expected_actors_(expected_actors) {}
  BarrierImpl(BarrierImpl const&) = delete;
  BarrierImpl& operator=(BarrierImpl const&) = delete;

  BarrierAcquisitionImplPtr acquire_async(actor::ActorImpl* issuer);
  unsigned get_id() const { return id_; }

  friend void intrusive_ptr_add_ref(BarrierImpl* barrier)
  {
    XBT_ATTRIB_UNUSED auto previous = barrier->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }

  friend void intrusive_ptr_release(BarrierImpl* barrier)
  {
    if (barrier->refcount_.fetch_sub(1) == 1)
      delete barrier;
  }

  s4u::Barrier& get_iface() { return piface_; }
};
} // namespace activity
} // namespace kernel
} // namespace simgrid
#endif
