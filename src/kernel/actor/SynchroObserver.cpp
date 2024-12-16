/* Copyright (c) 2019-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SynchroObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/activity/BarrierImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/backtrace.hpp"
#include "xbt/ex.h"
#include "xbt/log.h"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(obs_mutex, mc_observer, "Logging specific to mutex simcalls observation");

namespace simgrid::kernel::actor {

MutexObserver::MutexObserver(ActorImpl* actor, mc::Transition::Type type, activity::MutexImpl* mutex)
    : DelayedSimcallObserver<void>(actor), type_(type), mutex_(mutex)
{
  xbt_assert(mutex_);
}

void MutexObserver::serialize(std::stringstream& stream) const
{
  const auto* owner = get_mutex()->get_owner();
  stream << (short)type_ << ' ' << get_mutex()->get_id() << ' ' << (owner != nullptr ? owner->get_pid() : -1);
}
std::string MutexObserver::to_string() const
{
  return std::string(mc::Transition::to_c_str(type_)) + "(mutex_id:" + std::to_string(get_mutex()->get_id()) +
         " owner:" +
         (get_mutex()->get_owner() == nullptr ? "none" : std::to_string(get_mutex()->get_owner()->get_pid())) + ")";
}

bool MutexObserver::is_enabled()
{
  // Only wait can be disabled
  return type_ != mc::Transition::Type::MUTEX_WAIT || mutex_->get_owner() == get_issuer();
}

SemaphoreObserver::SemaphoreObserver(ActorImpl* actor, mc::Transition::Type type, activity::SemaphoreImpl* sem)
    : SimcallObserver(actor), type_(type), sem_(sem)
{
  xbt_assert(sem_);
}

void SemaphoreObserver::serialize(std::stringstream& stream) const
{
  stream << (short)type_ << ' ' << get_sem()->get_id() << ' ' << false /* Granted is ignored for LOCK/UNLOCK */ << ' '
         << get_sem()->get_capacity();
}
std::string SemaphoreObserver::to_string() const
{
  return std::string(mc::Transition::to_c_str(type_)) + "(sem_id:" + std::to_string(get_sem()->get_id()) + ")";
}

MutexAcquisitionObserver::MutexAcquisitionObserver(ActorImpl* actor, mc::Transition::Type type,
                                                   activity::MutexAcquisitionImpl* acqui, double timeout)
    : DelayedSimcallObserver(actor, false), type_(type), acquisition_(acqui), timeout_(timeout)
{
}
bool MutexAcquisitionObserver::is_enabled()
{
  return acquisition_->is_granted();
}
std::string MutexAcquisitionObserver::to_string() const
{
  if (acquisition_) {
    const auto* owner = acquisition_->get_mutex()->get_owner();
    return std::string(mc::Transition::to_c_str(type_)) +
           "(mutex_id:" + std::to_string(acquisition_->get_mutex()->get_id()) +
           " owner:" + (owner == nullptr ? "none" : std::to_string(owner->get_pid())) + ")";
  } else {
    return std::string(mc::Transition::to_c_str(type_)) + "(mutex_id:null)";
  }
}
void MutexAcquisitionObserver::serialize(std::stringstream& stream) const
{
  const auto* owner = acquisition_->get_mutex()->get_owner();
  stream << (short)type_ << ' ' << acquisition_->get_mutex()->get_id() << ' '
         << (owner != nullptr ? owner->get_pid() : -1);
}
SemaphoreAcquisitionObserver::SemaphoreAcquisitionObserver(ActorImpl* actor, mc::Transition::Type type,
                                                           activity::SemAcquisitionImpl* acqui, double timeout)
    : DelayedSimcallObserver(actor, false), type_(type), acquisition_(acqui), timeout_(timeout)
{
}
bool SemaphoreAcquisitionObserver::is_enabled()
{
  return acquisition_->granted_;
}
void SemaphoreAcquisitionObserver::serialize(std::stringstream& stream) const
{
  stream << (short)type_ << ' ' << acquisition_->semaphore_->get_id() << ' ' << acquisition_->granted_ << ' '
         << acquisition_->semaphore_->get_capacity();
}
std::string SemaphoreAcquisitionObserver::to_string() const
{
  if (type_ == mc::Transition::Type::SEM_LOCK_NOMC) // Out of MC, synchronous lock in one simcall
    return std::string(mc::Transition::to_c_str(type_)) + "()";

  return std::string(mc::Transition::to_c_str(type_)) +
         "(sem_id:" + std::to_string(acquisition_->semaphore_->get_id()) + ' ' +
         (acquisition_->granted_ ? "granted)" : "not granted)");
}

BarrierObserver::BarrierObserver(ActorImpl* actor, mc::Transition::Type type, activity::BarrierImpl* bar)
    : DelayedSimcallObserver(actor, false), type_(type), barrier_(bar), timeout_(-1)
{
  xbt_assert(type_ == mc::Transition::Type::BARRIER_ASYNC_LOCK);
}
BarrierObserver::BarrierObserver(ActorImpl* actor, mc::Transition::Type type, activity::BarrierAcquisitionImpl* acqui,
                                 double timeout)
    : DelayedSimcallObserver(actor, false), type_(type), acquisition_(acqui), timeout_(timeout)
{
  xbt_assert(type_ == mc::Transition::Type::BARRIER_WAIT);
}
void BarrierObserver::serialize(std::stringstream& stream) const
{
  xbt_assert(barrier_ != nullptr || (acquisition_ != nullptr && acquisition_->barrier_ != nullptr));
  stream << (short)type_ << ' ' << (barrier_ != nullptr ? barrier_->get_id() : acquisition_->barrier_->get_id());
}
std::string BarrierObserver::to_string() const
{
  return std::string(mc::Transition::to_c_str(type_)) + "(barrier_id:" +
         (barrier_ != nullptr ? std::to_string(barrier_->get_id())
                              : (acquisition_ != nullptr ? std::to_string(acquisition_->barrier_->get_id()) : "null")) +
         ")";
}
bool BarrierObserver::is_enabled()
{
  return type_ == mc::Transition::Type::BARRIER_ASYNC_LOCK ||
         (type_ == mc::Transition::Type::BARRIER_WAIT && acquisition_ != nullptr && acquisition_->granted_);
}

bool ConditionVariableObserver::is_enabled()
{
  return type_ != mc::Transition::Type::CONDVAR_WAIT || timeout_ > 0 || acquisition_->is_granted();
}
void ConditionVariableObserver::serialize(std::stringstream& stream) const
{
  switch (type_) {
    case mc::Transition::Type::CONDVAR_WAIT:
      stream << (short)type_ << ' ' << acquisition_->get_cond()->get_id() << ' ' << acquisition_->get_mutex()->get_id()
             << ' ' << acquisition_->is_granted() << ' ' << (timeout_ > 0);
      break;
    case mc::Transition::Type::CONDVAR_ASYNC_LOCK:
      stream << (short)type_ << ' ' << cond_->get_id() << ' ' << mutex_;
      break;
    case mc::Transition::Type::CONDVAR_SIGNAL:
    case mc::Transition::Type::CONDVAR_BROADCAST:
      stream << (short)type_ << ' ' << cond_->get_id();
      break;
    default:
      THROW_UNIMPLEMENTED;
  }
}
std::string ConditionVariableObserver::to_string() const
{
  if (type_ == mc::Transition::Type::CONDVAR_WAIT) {
    return std::string(mc::Transition::to_c_str(type_)) +
           "(cond_id: " + std::to_string(acquisition_->get_cond()->get_id()) +
           ", mutex_id:" + std::to_string(acquisition_->get_mutex()->get_id()) +
           ", timeout: " + (timeout_ >= 0 ? "yes" : "no") + ")";
  }
  if (type_ == mc::Transition::Type::CONDVAR_ASYNC_LOCK) {
    return std::string(mc::Transition::to_c_str(type_)) + "(cond_id: " + std::to_string(get_cond()->get_id()) +
           ", mutex_id:" +
           // mutex_ is only defined if acquisition_ is not (only if type != WAIT)
           std::to_string(get_mutex()->get_id()) + "timeout: " + (timeout_ >= 0 ? "yes" : "no") + ")";
  }

  if (type_ == mc::Transition::Type::CONDVAR_BROADCAST || type_ == mc::Transition::Type::CONDVAR_SIGNAL)
    return std::string(mc::Transition::to_c_str(type_)) + "(cond_id: " + std::to_string(get_cond()->get_id()) + ")";

  THROW_UNIMPLEMENTED;
}

} // namespace simgrid::kernel::actor
