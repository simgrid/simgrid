/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_config.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(obs_comm, mc_observer, "Logging specific to the Communication simcalls observation");

namespace simgrid {
namespace kernel {
namespace actor {

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
  stream << (uintptr_t)comm_ << ' ' << mbox_->get_id() << ' ' << (uintptr_t)src_buff_ << ' ' << src_buff_size_ << ' '
         << tag_;
  XBT_DEBUG("SendObserver comm:%p mbox:%u buff:%p size:%zu tag:%d", comm_, mbox_->get_id(), src_buff_, src_buff_size_,
            tag_);
}

void CommIrecvSimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::COMM_RECV << ' ';
  stream << (uintptr_t)comm_ << ' ' << mbox_->get_id() << ' ' << (uintptr_t)dst_buff_ << ' ' << tag_;
  XBT_DEBUG("RecvObserver comm:%p mbox:%u buff:%p tag:%d", comm_, mbox_->get_id(), dst_buff_, tag_);
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
