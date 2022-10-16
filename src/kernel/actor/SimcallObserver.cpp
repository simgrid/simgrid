/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/SimcallObserver.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
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

void RandomSimcall::prepare(int times_considered)
{
  next_value_ = min_ + times_considered;
  XBT_DEBUG("MC_RANDOM(%d, %d) will return %d after %d times", min_, max_, next_value_, times_considered);
}

int RandomSimcall::get_max_consider() const
{
  return max_ - min_ + 1;
}

bool ConditionWaitSimcall::is_enabled()
{
  if (static bool warned = false; not warned) {
    XBT_INFO("Using condition variables in model-checked code is still experimental. Use at your own risk");
    warned = true;
  }
  return true;
}
void ConditionWaitSimcall::serialize(std::stringstream& stream) const
{
  THROW_UNIMPLEMENTED;
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
  stream << other_->get_pid() << ' ' << static_cast<bool>(timeout_ > 0);
}
} // namespace simgrid::kernel::actor
