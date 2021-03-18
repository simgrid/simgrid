/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/SimcallObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_observer, mc, "Logging specific to MC simcall observation");

namespace simgrid {
namespace mc {

std::string SimcallObserver::to_string(int /*time_considered*/) const
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

std::string RandomSimcall::to_string(int time_considered) const
{
  return SimcallObserver::to_string(time_considered) + "MC_RANDOM(" + std::to_string(time_considered) + ")";
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

std::string MutexUnlockSimcall::to_string(int time_considered) const
{
  return SimcallObserver::to_string(time_considered) + "Mutex UNLOCK";
}

std::string MutexUnlockSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Mutex UNLOCK";
}

std::string MutexLockSimcall::to_string(int time_considered) const
{
  std::string res = SimcallObserver::to_string(time_considered) + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
  res += "(locked = " + std::to_string(mutex_->is_locked());
  res += ", owner = " + std::to_string(mutex_->get_owner() ? mutex_->get_owner()->get_pid() : -1);
  res += ", sleeping = n/a)";
  return res;
}

std::string MutexLockSimcall::dot_label() const
{
  return SimcallObserver::dot_label() + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
}

bool MutexLockSimcall::is_enabled() const
{
  return not blocking_ || mutex_->get_owner() == nullptr || mutex_->get_owner() == get_issuer();
}

std::string ConditionWaitSimcall::to_string(int time_considered) const
{
  std::string res = SimcallObserver::to_string(time_considered) + "Condition WAIT";
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

std::string SemAcquireSimcall::to_string(int time_considered) const
{
  std::string res = SimcallObserver::to_string(time_considered) + "Sem ACQUIRE";
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

std::string ExecutionWaitanySimcall::to_string(int time_considered) const
{
  std::string res = SimcallObserver::to_string(time_considered) + "Execution WAITANY";
  res += "(" + (timeout_ == -1.0 ? "" : std::to_string(timeout_)) + ")";
  return res;
}

std::string ExecutionWaitanySimcall::dot_label() const
{
  return SimcallObserver::dot_label() + "Execution WAITANY";
}
} // namespace mc
} // namespace simgrid
