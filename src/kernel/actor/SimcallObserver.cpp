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

bool SimcallObserver::depends(SimcallObserver* other)
{
  THROW_UNIMPLEMENTED;
}
/* Random is only dependent when issued by the same actor (ie, always independent) */
bool RandomSimcall::depends(SimcallObserver* other)
{
  return get_issuer() == other->get_issuer();
}
void RandomSimcall::serialize(Simcall& type, char* buffer)
{
  type = Simcall::RANDOM;
  std::stringstream stream;

  stream << min_ << ' ' << max_;
  strcpy(buffer, stream.str().c_str());
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

/*
std::string SimcallObserver::to_string(int) const
{
  return simgrid::xbt::string_printf("[(%ld)%s (%s)] ", issuer_->get_pid(), issuer_->get_host()->get_cname(),
                                     issuer_->get_cname());
}*/

std::string SimcallObserver::dot_label(int /*times_considered*/) const
{
  if (issuer_->get_host())
    return xbt::string_printf("[(%ld)%s] ", issuer_->get_pid(), issuer_->get_host()->get_cname());
  return xbt::string_printf("[(%ld)] ", issuer_->get_pid());
}

std::string RandomSimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) + "MC_RANDOM(" + std::to_string(next_value_) + ")";
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

std::string MutexUnlockSimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) + "Mutex UNLOCK";
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

std::string MutexLockSimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) + (blocking_ ? "Mutex LOCK" : "Mutex TRYLOCK");
}

bool MutexLockSimcall::is_enabled() const
{
  return not blocking_ || get_mutex()->get_owner() == nullptr || get_mutex()->get_owner() == get_issuer();
}

std::string ConditionWaitSimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) + "Condition WAIT";
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

std::string SemAcquireSimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) + "Sem ACQUIRE";
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

int ActivityTestanySimcall::get_max_consider() const
{
  // Only Comms are of interest to MC for now. When all types of activities can be consider, this function can simply
  // return the size of activities_.
  int count = 0;
  for (const auto& act : activities_)
    if (dynamic_cast<activity::CommImpl*>(act) != nullptr)
      count++;
  return count;
}

void ActivityTestanySimcall::prepare(int times_considered)
{
  next_value_ = times_considered;
}

/*
std::string ActivityTestanySimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered);
  if (times_considered == -1) {
    res += "TestAny FALSE(-)";
  } else {
    res += "TestAny(" + xbt::string_printf("(%d of %zu)", times_considered + 1, activities_.size());
  }

  return res;
}*/

std::string ActivityTestanySimcall::dot_label(int times_considered) const
{
  std::string res = SimcallObserver::dot_label(times_considered) + "TestAny ";
  if (times_considered == -1) {
    res += "FALSE";
  } else {
    res += xbt::string_printf("TRUE [%d of %zu]", times_considered + 1, activities_.size());
  }
  return res;
}

bool ActivityTestSimcall::depends(SimcallObserver* other)
{
  if (get_issuer() == other->get_issuer())
    return false;

  if (dynamic_cast<ActivityTestSimcall*>(other))
    return true;

  const auto* comm1 = dynamic_cast<activity::CommImpl*>(activity_);
  if (comm1 == nullptr)
    return false;

  if (dynamic_cast<ActivityWaitSimcall*>(other) != nullptr &&
      (comm1->src_actor_.get() == nullptr || comm1->dst_actor_.get() == nullptr))
    return false;

  if (comm1->src_buff_ == nullptr || comm1->dst_buff_ == nullptr)
    return false;

  if (const auto* test = dynamic_cast<ActivityTestSimcall*>(other)) {
    const auto* comm2 = dynamic_cast<activity::CommImpl*>(test->get_activity());
    if (comm2 == nullptr)
      return false;
    else if (comm2->src_buff_ == nullptr || comm2->dst_buff_ == nullptr)
      return false;
  }

  if (auto* wait = dynamic_cast<ActivityWaitSimcall*>(other)) {
    auto* comm2 = dynamic_cast<activity::CommImpl*>(wait->get_activity());
    if (comm2 == nullptr)
      return false;
    if (comm1->src_buff_ == comm2->src_buff_ && comm1->dst_buff_ == comm2->dst_buff_)
      return false;
    if (comm1->src_buff_ != nullptr && comm1->dst_buff_ != nullptr && comm2->src_buff_ != nullptr &&
        comm2->dst_buff_ != nullptr && comm1->dst_buff_ != comm2->src_buff_ && comm1->dst_buff_ != comm2->dst_buff_ &&
        comm2->dst_buff_ != comm1->src_buff_)
      return false;
  }

  return true;
}
void ActivityWaitSimcall::serialize(Simcall& type, char* buffer)
{
  std::stringstream stream;
  if (auto* comm = dynamic_cast<activity::CommImpl*>(activity_)) {
    type = Simcall::COMM_WAIT;
    stream << (timeout_ > 0) << ' ' << comm;
    stream << ' ' << (comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1);
    stream << ' ' << (comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1);
    stream << ' ' << comm->get_mailbox_id();
    stream << ' ' << (void*)comm->src_buff_ << ' ' << (void*)comm->dst_buff_ << ' ' << comm->src_buff_size_;
    strcpy(buffer, stream.str().c_str());
  } else {
    type = Simcall::UNKNOWN;
    strcpy(buffer, stream.str().c_str());
  }
}

/*
std::string ActivityTestSimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "Test ";
  if (const auto* comm = dynamic_cast<activity::CommImpl*>(activity_)) {
    if (comm->src_actor_.get() == nullptr || comm->dst_actor_.get() == nullptr) {
      res += "FALSE(comm=";
      res += XBT_LOG_ISENABLED(mc_observer, xbt_log_priority_verbose) ? xbt::string_printf("%p)", comm)
                                                                      : "(verbose only))";
    } else {
      res += "TRUE(comm=";

      auto src = comm->src_actor_;
      auto dst = comm->dst_actor_;
      res +=
          XBT_LOG_ISENABLED(mc_observer, xbt_log_priority_verbose) ? xbt::string_printf("%p", comm) : "(verbose only) ";
      res += xbt::string_printf("[(%ld)%s (%s) ", src->get_pid(), src->get_host()->get_cname(), src->get_cname()) +
             "-> " +
             xbt::string_printf("(%ld)%s (%s)])", dst->get_pid(), dst->get_host()->get_cname(), dst->get_cname());
    }
  } else
    xbt_die("Only Comms are supported here for now");
  return res;
}*/

std::string ActivityTestSimcall::dot_label(int times_considered) const
{
  std::string res  = SimcallObserver::dot_label(times_considered) + "Test ";
  const auto* comm = dynamic_cast<activity::CommImpl*>(activity_);
  if (comm && (comm->src_actor_.get() == nullptr || comm->dst_actor_.get() == nullptr)) {
    res += "FALSE";
  } else {
    res += "TRUE";
  }
  return res;
}

bool ActivityWaitSimcall::is_enabled() const
{
  /* FIXME: check also that src and dst processes are not suspended */
  const auto* comm = dynamic_cast<activity::CommImpl*>(activity_);
  if (comm == nullptr)
    xbt_die("Only Comms are supported here for now");

  if (comm->src_timeout_ || comm->dst_timeout_) {
    /* If it has a timeout it will be always be enabled (regardless of who declared the timeout),
     * because even if the communication is not ready, it can timeout and won't block. */
    if (_sg_mc_timeout == 1)
      return true;
  }
  /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
  else if (comm->detached() && comm->src_actor_ == nullptr && comm->get_state() == activity::State::READY)
    return (comm->dst_actor_ != nullptr);
  return (comm->src_actor_ && comm->dst_actor_);
}

std::string ActivityWaitSimcall::dot_label(int times_considered) const
{
  std::string res = SimcallObserver::dot_label(times_considered);
  res += (times_considered == -1) ? "WaitTimeout " : "Wait ";

  const auto* comm = dynamic_cast<activity::CommImpl*>(activity_);
  if (comm) {
    auto src = comm->src_actor_;
    auto dst = comm->dst_actor_;
    res += " [(" + std::to_string(src ? src->get_pid() : 0) + ")";
    res += "->(" + std::to_string(dst ? dst->get_pid() : 0) + ")]";
  } else
    xbt_die("Only Comms are supported here for now");
  return res;
}

std::string ActivityWaitanySimcall::dot_label(int times_considered) const
{
  return SimcallObserver::dot_label(times_considered) +
         xbt::string_printf("WaitAny [%d of %zu]", times_considered + 1, activities_.size());
}

bool ActivityWaitanySimcall::is_enabled() const
{
  // FIXME: deal with other kind of activities (Exec and I/Os)
  // FIXME: Can be factored with ActivityWaitSimcall::is_enabled()
  const auto* comm = dynamic_cast<activity::CommImpl*>(activities_[next_value_]);
  if (comm == nullptr)
    xbt_die("Only Comms are supported here for now");
  if (comm->src_timeout_ || comm->dst_timeout_) {
    /* If it has a timeout it will be always be enabled (regardless of who declared the timeout),
     * because even if the communication is not ready, it can timeout and won't block. */
    if (_sg_mc_timeout == 1)
      return true;
  }
  /* On the other hand if it hasn't a timeout, check if the comm is ready.*/
  else if (comm->detached() && comm->src_actor_ == nullptr && comm->get_state() == activity::State::READY)
    return (comm->dst_actor_ != nullptr);
  return (comm->src_actor_ && comm->dst_actor_);
}

int ActivityWaitanySimcall::get_max_consider() const
{
  return static_cast<int>(activities_.size());
}

void ActivityWaitanySimcall::prepare(int times_considered)
{
  next_value_ = times_considered;
}

void CommIsendSimcall::serialize(Simcall& type, char* buffer)
{
  type = Simcall::ISEND;
  std::stringstream stream;
  stream << mbox_->get_id() << ' ' << (void*)src_buff_ << ' ' << src_buff_size_;
  strcpy(buffer, stream.str().c_str());
  XBT_DEBUG("SendObserver mbox:%u buff:%p size:%zu", mbox_->get_id(), src_buff_, src_buff_size_);
}

void CommIrecvSimcall::serialize(Simcall& type, char* buffer)
{
  type = Simcall::IRECV;
  std::stringstream stream;
  stream << mbox_->get_id() << dst_buff_;
  strcpy(buffer, stream.str().c_str());
}


/*
std::string CommIrecvSimcall::to_string(int times_considered) const
{
  std::string res = SimcallObserver::to_string(times_considered) + "iRecv(";
  res += xbt::string_printf("dst=(%ld)%s (%s)", get_issuer()->get_pid(), get_issuer()->get_host()->get_cname(),
                            get_issuer()->get_cname());
  res += ", buff=" + (XBT_LOG_ISENABLED(mc_observer, xbt_log_priority_verbose) ? xbt::string_printf("%p", dst_buff_)
                                                                               : "(verbose only)");
  res += ", size=" + (XBT_LOG_ISENABLED(mc_observer, xbt_log_priority_verbose) ? std::to_string(*dst_buff_size_)
                                                                               : "(verbose only)");
  res += ")";
  return res;
}
*/

} // namespace actor
} // namespace kernel
} // namespace simgrid
