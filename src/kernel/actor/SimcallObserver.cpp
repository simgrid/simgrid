/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SimcallObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_config.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_observer, mc, "Logging specific to MC simcall observation");

namespace simgrid {
namespace kernel {
namespace actor {

void SimcallObserver::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::UNKNOWN;
}
bool SimcallObserver::depends(SimcallObserver* other)
{
  THROW_UNIMPLEMENTED;
}
/* Random is only dependent when issued by the same actor (ie, always independent) */
bool RandomSimcall::depends(SimcallObserver* other)
{
  return get_issuer() == other->get_issuer();
}
void RandomSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::RANDOM << ' ';
  stream << min_ << ' ' << max_;
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

void RandomSimcall::prepare(int times_considered)
{
  next_value_ = min_ + times_considered;
  XBT_DEBUG("MC_RANDOM(%d, %d) will return %d after %d times", min_, max_, next_value_, times_considered);
}

int RandomSimcall::get_max_consider()
{
  return max_ - min_ + 1;
}

/*
std::string MutexLockSimcall::to_string(int times_considered) const
{
  auto mutex      = get_mutex();
  std::string res = SimcallObserver::to_string(times_considered) + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
  res += "(locked = " + std::to_string(mutex->is_locked());
  res += ", owner = " + std::to_string(mutex->get_owner() ? mutex->get_owner()->get_pid() : -1);
  res += ", sleeping = n/a)";
  return res;
}*/

bool MutexLockSimcall::is_enabled()
{
  return not blocking_ || get_mutex()->get_owner() == nullptr || get_mutex()->get_owner() == get_issuer();
}

bool ConditionWaitSimcall::is_enabled()
{
  static bool warned = false;
  if (not warned) {
    XBT_INFO("Using condition variables in model-checked code is still experimental. Use at your own risk");
    warned = true;
  }
  return true;
}

bool SemAcquireSimcall::is_enabled()
{
  static bool warned = false;
  if (not warned) {
    XBT_INFO("Using semaphore in model-checked code is still experimental. Use at your own risk");
    warned = true;
  }
  return true;
}

ActivityTestanySimcall::ActivityTestanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities)
    : ResultingSimcall(actor, -1), activities_(activities)
{
}

int ActivityTestanySimcall::get_max_consider()
{
  indexes_.clear();
  // list all the activities that are ready
  for (unsigned i = 0; i < activities_.size(); i++)
    if (activities_[i]->test(get_issuer()))
      indexes_.push_back(i);
  return indexes_.size() + 1;
}

void ActivityTestanySimcall::prepare(int times_considered)
{
  if (times_considered < static_cast<int>(indexes_.size()))
    next_value_ = indexes_.at(times_considered);
  else
    next_value_ = -1;
}
static void serialize_activity_test(const activity::ActivityImpl* act, std::stringstream& stream)
{
  if (auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    stream << "  " << (short)mc::Transition::Type::COMM_TEST;
    stream << ' ' << (uintptr_t)comm;
    stream << ' ' << (comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1);
    stream << ' ' << (comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1);
    stream << ' ' << comm->get_mailbox_id();
    stream << ' ' << (uintptr_t)comm->src_buff_ << ' ' << (uintptr_t)comm->dst_buff_ << ' ' << comm->src_buff_size_;
  } else {
    stream << (short)mc::Transition::Type::UNKNOWN;
  }
}
void ActivityTestanySimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::TESTANY << ' ' << activities_.size() << ' ';
  for (auto const& act : activities_) {
    serialize_activity_test(act, stream);
    stream << ' ';
  }
}
void ActivityTestSimcall::serialize(std::stringstream& stream) const
{
  serialize_activity_test(activity_, stream);
}
static void serialize_activity_wait(const activity::ActivityImpl* act, bool timeout, std::stringstream& stream)
{
  if (auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    stream << (short)mc::Transition::Type::COMM_WAIT << ' ';
    stream << timeout << ' ' << (uintptr_t)comm;

    stream << ' ' << (comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1);
    stream << ' ' << (comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1);
    stream << ' ' << comm->get_mailbox_id();
    stream << ' ' << (uintptr_t)comm->src_buff_ << ' ' << (uintptr_t)comm->dst_buff_ << ' ' << comm->src_buff_size_;
  } else {
    stream << (short)mc::Transition::Type::UNKNOWN;
  }
}

void ActivityWaitSimcall::serialize(std::stringstream& stream) const
{
  serialize_activity_wait(activity_, timeout_ > 0, stream);
}
void ActivityWaitanySimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::WAITANY << ' ' << activities_.size() << ' ';
  for (auto const& act : activities_) {
    serialize_activity_wait(act, timeout_ > 0, stream);
    stream << ' ';
  }
}
ActivityWaitanySimcall::ActivityWaitanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities,
                                               double timeout)
    : ResultingSimcall(actor, -1), activities_(activities), timeout_(timeout)
{
}

bool ActivityWaitSimcall::is_enabled()
{
  // FIXME: if _sg_mc_timeout == 1 and if we have either a sender or receiver timeout, the transition is enabled
  // because even if the communication is not ready, it can timeout and won't block.

  return activity_->test(get_issuer());
}

bool ActivityWaitanySimcall::is_enabled()
{
  // list all the activities that are ready
  indexes_.clear();
  for (unsigned i = 0; i < activities_.size(); i++)
    if (activities_[i]->test(get_issuer()))
      indexes_.push_back(i);

  //  if (_sg_mc_timeout && timeout_)  FIXME: deal with the potential timeout of the WaitAny

  // FIXME: even if the WaitAny has no timeout, some of the activities may still have one.
  // we should iterate over the vector searching for them
  return not indexes_.empty();
}

int ActivityWaitanySimcall::get_max_consider()
{
  // list all the activities that are ready
  indexes_.clear();
  for (unsigned i = 0; i < activities_.size(); i++)
    if (activities_[i]->test(get_issuer()))
      indexes_.push_back(i);

  int res = indexes_.size();
  //  if (_sg_mc_timeout && timeout_)
  //    res++;

  return res;
}

void ActivityWaitanySimcall::prepare(int times_considered)
{
  if (times_considered < static_cast<int>(indexes_.size()))
    next_value_ = indexes_.at(times_considered);
  else
    next_value_ = -1;
}

void CommIsendSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::COMM_SEND << ' ';
  stream << (uintptr_t)comm_ << ' ' << mbox_->get_id() << ' ' << (uintptr_t)src_buff_ << ' ' << src_buff_size_;
  XBT_DEBUG("SendObserver mbox:%u buff:%p size:%zu", mbox_->get_id(), src_buff_, src_buff_size_);
}

void CommIrecvSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::COMM_RECV << ' ';
  stream << (uintptr_t)comm_ << ' ' << mbox_->get_id() << ' ' << (uintptr_t)dst_buff_;
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
