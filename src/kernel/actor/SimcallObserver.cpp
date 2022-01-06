/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SimcallObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_observer, mc, "Logging specific to MC simcall observation");

namespace simgrid {
namespace kernel {
namespace actor {

bool SimcallObserver::depends(SimcallObserver* other)
{
  THROW_UNIMPLEMENTED;
}
/* Random is only dependent when issued by the same actor (ie, always independent) */
bool RandomSimcall::depends(SimcallObserver* other)
{
  return get_issuer() == other->get_issuer();
}
bool MutexSimcall::depends(SimcallObserver* other)
{
  if (dynamic_cast<RandomSimcall*>(other) != nullptr)
    return other->depends(this); /* Other is random, that is very permissive. Use that relation instead. */

#if 0 /* This code is currently broken and shouldn't be used. We must implement asynchronous locks before */
  MutexSimcall* that = dynamic_cast<MutexSimcall*>(other);
  if (that == nullptr)
    return true; // Depends on anything we don't know

  /* Theorem 4.4.7: Any pair of synchronization actions of distinct actors concerning distinct mutexes are independent */
  if (this->get_issuer() != that->get_issuer() && this->get_mutex() != that->get_mutex())
    return false;

  /* Theorem 4.4.8 An AsyncMutexLock is independent with a MutexUnlock of another actor */
  if (((dynamic_cast<MutexLockSimcall*>(this) != nullptr && dynamic_cast<MutexUnlockSimcall*>(that)) ||
       (dynamic_cast<MutexLockSimcall*>(that) != nullptr && dynamic_cast<MutexUnlockSimcall*>(this))) &&
      get_issuer() != other->get_issuer())
    return false;
#endif
  return true; // Depend on things we don't know for sure that they are independent
}

std::string SimcallObserver::to_string(int /*times_considered*/) const
{
  return simgrid::xbt::string_printf("[(%ld)%s (%s)] ", issuer_->get_pid(), issuer_->get_host()->get_cname(),
                                     issuer_->get_cname());
}

std::string SimcallObserver::dot_label() const
{
  if (issuer_->get_host())
    return xbt::string_printf("[(%ld)%s] ", issuer_->get_pid(), issuer_->get_cname());
  return xbt::string_printf("[(%ld)] ", issuer_->get_pid());
}

std::string RandomSimcall::to_string(int times_considered) const
{
  return SimcallObserver::to_string(times_considered) + "MC_RANDOM(" + std::to_string(times_considered) + ")";
}

std::string RandomSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "MC_RANDOM(" + std::to_string(next_value_) + ")";
}

void RandomSimcall::prepare(int times_considered)
{
  next_value_ = min_ + times_considered;
  XBT_DEBUG("MC_RANDOM(%d, %d) will return %d after %d times", min_, max_, next_value_, times_considered);
}

int RandomSimcall::get_max_consider() const
{
  return max_ - min_ + 1;
}

std::string MutexUnlockSimcall::to_string(int times_considered) const
{
  return SimcallObserver::to_string(times_considered) + "Mutex UNLOCK";
}

std::string MutexUnlockSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Mutex UNLOCK";
}

std::string MutexLockSimcall::to_string(int times_considered) const
{
  auto mutex      = get_mutex();
  std::string res = SimcallObserver::to_string(times_considered) + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
  res += "(locked = " + std::to_string(mutex->is_locked());
  res += ", owner = " + std::to_string(mutex->get_owner() ? mutex->get_owner()->get_pid() : -1);
  res += ", sleeping = n/a)";
  return res;
}

std::string MutexLockSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
}

bool MutexLockSimcall::is_enabled() const
{
  return not blocking_ || get_mutex()->get_owner() == nullptr || get_mutex()->get_owner() == get_issuer();
}

std::string ConditionWaitSimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "Condition WAIT";
  res += "(" + (timeout_ == -1.0 ? "" : std::to_string(timeout_)) + ")";
  return res;
}

std::string ConditionWaitSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Condition WAIT";
}

bool ConditionWaitSimcall::is_enabled() const
{
  static bool warned = false;
  if (not warned) {
    XBT_INFO("Using condition variables in model-checked code is still experimental. Use at your own risk");
    warned = true;
  }
  return true;
}

std::string SemAcquireSimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "Sem ACQUIRE";
  res += "(" + (timeout_ == -1.0 ? "" : std::to_string(timeout_)) + ")";
  return res;
}

std::string SemAcquireSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Sem ACQUIRE";
}

bool SemAcquireSimcall::is_enabled() const
{
  static bool warned = false;
  if (not warned) {
    XBT_INFO("Using semaphore in model-checked code is still experimental. Use at your own risk");
    warned = true;
  }
  return true;
}

std::string ExecutionWaitanySimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "Execution WAITANY";
  res += "(" + (timeout_ == -1.0 ? "" : std::to_string(timeout_)) + ")";
  return res;
}

std::string ExecutionWaitanySimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Execution WAITANY";
}

std::string IoWaitanySimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "I/O WAITANY";
  res += "(" + (timeout_ == -1.0 ? "" : std::to_string(timeout_)) + ")";
  return res;
}

std::string IoWaitanySimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "I/O WAITANY";
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
