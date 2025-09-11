/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SimcallObserver.hpp"
#include "simgrid/forward.h"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/remote/Channel.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_observer, mc, "Logging specific to MC simcall observation");

namespace simgrid::kernel::actor {

void RandomSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::RANDOM);
  channel.pack<int>(min_);
  channel.pack<int>(max_);
}
std::string RandomSimcall::to_string() const
{
  return "Random(min:" + std::to_string(min_) + " max:" + std::to_string(max_) + ")";
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

ActorJoinSimcall::ActorJoinSimcall(ActorImpl* actor, ActorImpl* other, double timeout)
    : DelayedSimcallObserver(actor), other_(s4u::ActorPtr(other->get_iface())), timeout_(timeout)
{
}
bool ActorJoinSimcall::is_enabled()
{
  return other_->get_impl()->wannadie();
}
void ActorJoinSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::ACTOR_JOIN);
  channel.pack<aid_t>(other_->get_pid());
  channel.pack<bool>((timeout_ > 0));
}
std::string ActorJoinSimcall::to_string() const
{
  return "ActorJoin(pid:" + std::to_string(other_->get_pid()) + ")";
}
ActorExitSimcall::ActorExitSimcall(ActorImpl* actor) : SimcallObserver(actor) {}
void ActorExitSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::ACTOR_EXIT);
}
std::string ActorExitSimcall::to_string() const
{
  return "ActorExit()";
}
void ActorSleepSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::ACTOR_SLEEP);
}

std::string ActorSleepSimcall::to_string() const
{
  return "ActorSleep()";
}

void ActorCreateSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::ACTOR_CREATE);
  channel.pack<aid_t>(child_);
}

std::string ActorCreateSimcall::to_string() const
{
  return "ActorCreate(" + std::to_string(child_) + ")";
}

void ObjectAccessSimcallObserver::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::OBJECT_ACCESS);
  channel.pack(object_);
  channel.pack(get_owner()->get_pid());
}
std::string ObjectAccessSimcallObserver::to_string() const
{
  return "ObjectAccess(obj:" + ptr_to_id<ObjectAccessSimcallItem const>(object_) +
         " owner:" + std::to_string(get_owner()->get_pid()) + ")";
}
bool ObjectAccessSimcallObserver::is_visible() const
{
  return get_owner() != get_issuer();
}
ActorImpl* ObjectAccessSimcallObserver::get_owner() const
{
  return object_->simcall_owner_;
}

} // namespace simgrid::kernel::actor
