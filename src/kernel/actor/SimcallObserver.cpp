/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

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

namespace simgrid::kernel::actor {

void RandomSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::RANDOM << ' ';
  stream << min_ << ' ' << max_;
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
    : SimcallObserver(actor), other_(s4u::ActorPtr(other->get_iface())), timeout_(timeout)
{
}
bool ActorJoinSimcall::is_enabled()
{
  return other_->get_impl()->wannadie();
}
void ActorJoinSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::ACTOR_JOIN << ' ';
  stream << other_->get_pid() << ' ' << (timeout_ > 0);
}
std::string ActorJoinSimcall::to_string() const
{
  return "ActorJoin(pid:" + std::to_string(other_->get_pid()) + ")";
}
void ActorSleepSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::ACTOR_SLEEP;
}

std::string ActorSleepSimcall::to_string() const
{
  return "ActorSleep()";
}

void ObjectAccessSimcallObserver::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::OBJECT_ACCESS << ' ';
  stream << object_ << ' ' << get_owner()->get_pid();
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
