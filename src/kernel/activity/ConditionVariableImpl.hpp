/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLE_HPP
#define SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLE_HPP

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

#include <boost/intrusive/list.hpp>

namespace simgrid::kernel::activity {

class XBT_PUBLIC ConditionVariableAcquisitionImpl : public ActivityImpl_T<ConditionVariableAcquisitionImpl> {
  actor::ActorImpl* issuer_    = nullptr;
  MutexImpl* mutex_            = nullptr;
  ConditionVariableImpl* cond_ = nullptr;
  bool granted_                = false;
  bool mc_timeout_             = false; // Whether we are in MC mode && a timeout was declared

  friend ConditionVariableImpl;

public:
  ConditionVariableAcquisitionImpl(actor::ActorImpl* issuer, ConditionVariableImpl* condvar, MutexImpl* mutex);
  MutexImplPtr get_mutex() const { return mutex_; }
  ConditionVariableImplPtr get_cond() const { return cond_; }
  actor::ActorImpl* get_issuer() const { return issuer_; }
  void grant() { granted_ = true; }
  bool is_granted() const { return granted_; }

  bool test(actor::ActorImpl* issuer = nullptr) override { return granted_; }
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  void finish() override;
  void cancel() override;
};

class XBT_PUBLIC ConditionVariableImpl {
  s4u::ConditionVariable piface_;
  std::deque<ConditionVariableAcquisitionImplPtr> ongoing_acquisitions_;

  static unsigned next_id_;
  const unsigned id_ = next_id_++;

  std::atomic_int_fast32_t refcount_{1};
  friend void intrusive_ptr_add_ref(ConditionVariableImpl* cond);
  friend void intrusive_ptr_release(ConditionVariableImpl* cond);

  friend ConditionVariableAcquisitionImpl;

public:
  ConditionVariableImpl() : piface_(this){};

  const s4u::ConditionVariable* get_iface() const { return &piface_; }
  s4u::ConditionVariable* get_iface() { return &piface_; }
  unsigned get_id() const { return id_; }

  ConditionVariableAcquisitionImplPtr acquire_async(actor::ActorImpl* issuer, MutexImpl* mutex);
  void broadcast();
  void signal();
};
} // namespace simgrid::kernel::activity

#endif
