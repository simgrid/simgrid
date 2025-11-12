/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MUTEX_OBSERVER_HPP
#define SIMGRID_MC_MUTEX_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"

#include <string>

namespace simgrid::kernel::actor {

/* All the observers of Mutex transitions are very similar, so implement them all together in this class */
class MutexObserver final : public DelayedSimcallObserver<void> {
  mc::Transition::Type type_;
  activity::MutexImpl* const mutex_;

public:
  MutexObserver(ActorImpl* actor, mc::Transition::Type type, activity::MutexImpl* mutex);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;

  activity::MutexImpl* get_mutex() const { return mutex_; }
};

/* This observer is used for SEM_LOCK and SEM_UNLOCK (only) */
class SemaphoreObserver final : public SimcallObserver {
  mc::Transition::Type type_;
  activity::SemaphoreImpl* const sem_;

public:
  SemaphoreObserver(ActorImpl* actor, mc::Transition::Type type, activity::SemaphoreImpl* sem);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;

  activity::SemaphoreImpl* get_sem() const { return sem_; }
};

/* This observer is used for MUTEX_WAIT, that is returning and needs the acquisition (in MC mode) */
class MutexAcquisitionObserver final : public DelayedSimcallObserver<bool> {
  mc::Transition::Type type_;
  activity::MutexAcquisitionImpl* const acquisition_;
  const double timeout_;

public:
  MutexAcquisitionObserver(ActorImpl* actor, mc::Transition::Type type, activity::MutexAcquisitionImpl* acqui,
                           double timeout = -1.0);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;

  double get_timeout() const { return timeout_; }
};

/* This observer is used for SEM_WAIT, that is returning and needs the acquisition (in MC mode) */
class SemaphoreAcquisitionObserver final : public DelayedSimcallObserver<bool> {
  mc::Transition::Type type_;
  activity::SemAcquisitionImpl* const acquisition_;
  const double timeout_;

public:
  SemaphoreAcquisitionObserver(ActorImpl* actor, mc::Transition::Type type, activity::SemAcquisitionImpl* acqui,
                               double timeout = -1.0);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;

  double get_timeout() const { return timeout_; }
};

/* This observer is used for BARRIER_LOCK and BARRIER_WAIT. WAIT is returning and needs the acquisition */
class BarrierObserver final : public DelayedSimcallObserver<bool> {
  mc::Transition::Type type_;
  activity::BarrierImpl* const barrier_                = nullptr;
  activity::BarrierAcquisitionImpl* const acquisition_ = nullptr;
  const double timeout_;

public:
  BarrierObserver(ActorImpl* actor, mc::Transition::Type type, activity::BarrierImpl* bar);
  BarrierObserver(ActorImpl* actor, mc::Transition::Type type, activity::BarrierAcquisitionImpl* acqui,
                  double timeout = -1.0);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;

  double get_timeout() const { return timeout_; }
};

class ConditionVariableObserver final : public DelayedSimcallObserver<bool> {
  mc::Transition::Type type_;
  activity::ConditionVariableImplPtr const cond_ = nullptr;
  activity::MutexImplPtr const mutex_            = nullptr;

  activity::ConditionVariableAcquisitionImpl* const acquisition_ = nullptr;
  const double timeout_                                          = -1;

public:
  ConditionVariableObserver(ActorImpl* actor, mc::Transition::Type type, activity::ConditionVariableImpl* cond,
                            activity::MutexImpl* mutex, double timeout = -1.0)
      : DelayedSimcallObserver(actor, false), type_(type), cond_(cond), mutex_(mutex), timeout_(timeout)
  {
    xbt_assert(type == mc::Transition::Type::CONDVAR_ASYNC_LOCK || type == mc::Transition::Type::CONDVAR_NOMC,
               "With no acquisition, that must be a lock or a noMC");
  }
  ConditionVariableObserver(ActorImpl* actor, mc::Transition::Type type,
                            activity::ConditionVariableAcquisitionImpl* acqui, double timeout = -1.0)
      : DelayedSimcallObserver(actor, false)
      , type_(type)
      , cond_(acqui->get_cond())
      , mutex_(acqui->get_mutex())
      , acquisition_(acqui)
      , timeout_(timeout)
  {
    xbt_assert(type == mc::Transition::Type::CONDVAR_WAIT, "If you provide an acquisition, that must be a wait");
  }
  ConditionVariableObserver(ActorImpl* actor, mc::Transition::Type type, activity::ConditionVariableImpl* cond)
      : DelayedSimcallObserver(actor, false), type_(type), cond_(cond)
  {
    xbt_assert(type == mc::Transition::Type::CONDVAR_SIGNAL || type == mc::Transition::Type::CONDVAR_BROADCAST);
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;
  activity::ConditionVariableImpl* get_cond() const { return cond_.get(); }
  activity::MutexImpl* get_mutex() const { return mutex_.get(); }
  double get_timeout() const { return timeout_; }
  mc::Transition::Type get_type() const { return type_; }
};

} // namespace simgrid::kernel::actor

#endif
