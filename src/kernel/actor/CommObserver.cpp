/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

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

namespace simgrid::kernel::actor {

ActivityTestanySimcall::ActivityTestanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities)
    : ResultingSimcall(actor, -1), activities_(activities)
{
  indexes_.clear();
  // list all the activities that are ready
  for (unsigned i = 0; i < activities_.size(); i++)
    if (activities_[i]->test(get_issuer()))
      indexes_.push_back(i);
}

int ActivityTestanySimcall::get_max_consider() const
{
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
  if (const auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    stream << "  " << (short)mc::Transition::Type::COMM_TEST;
    stream << ' ' << comm->get_id();
    stream << ' ' << (comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1);
    stream << ' ' << (comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1);
    stream << ' ' << comm->get_mailbox_id();
    stream << ' ' << (uintptr_t)comm->src_buff_ << ' ' << (uintptr_t)comm->dst_buff_ << ' ' << comm->src_buff_size_;
  } else {
    stream << (short)mc::Transition::Type::UNKNOWN;
  }
}
static std::string to_string_activity_test(const activity::ActivityImpl* act)
{
  if (const auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    const std::string src_buff_id = ptr_to_id<unsigned char>(comm->src_buff_);
    const std::string dst_buff_id = ptr_to_id<unsigned char>(comm->dst_buff_);
    return "CommTest(comm_id:" + std::to_string(comm->get_id()) +
           " src:" + std::to_string(comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1) +
           " dst:" + std::to_string(comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1) +
           " mbox:" + std::to_string(comm->get_mailbox_id()) + " srcbuf:" + src_buff_id + " dstbuf:" + dst_buff_id +
           " bufsize:" + std::to_string(comm->src_buff_size_);
  } else {
    return "TestUnknownType()";
  }
}
void ActivityTestanySimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::TESTANY << ' ' << activities_.size() << ' ';
  for (auto const* act : activities_) {
    serialize_activity_test(act, stream);
    stream << ' ';
  }
}
std::string ActivityTestanySimcall::to_string() const
{
  std::stringstream buffer("TestAny(");
  for (auto const* act : activities_) {
    buffer << to_string_activity_test(act);
  }
  return buffer.str();
}

void ActivityTestSimcall::serialize(std::stringstream& stream) const
{
  serialize_activity_test(activity_, stream);
}
std::string ActivityTestSimcall::to_string() const
{
  return to_string_activity_test(activity_);
}
static void serialize_activity_wait(const activity::ActivityImpl* act, bool timeout, std::stringstream& stream)
{
  if (const auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    stream << (short)mc::Transition::Type::COMM_WAIT << ' ';
    stream << timeout << ' ' << comm->get_id();

    stream << ' ' << (comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1);
    stream << ' ' << (comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1);
    stream << ' ' << comm->get_mailbox_id();
    stream << ' ' << (uintptr_t)comm->src_buff_ << ' ' << (uintptr_t)comm->dst_buff_ << ' ' << comm->src_buff_size_;
  } else {
    stream << (short)mc::Transition::Type::UNKNOWN;
  }
}
static std::string to_string_activity_wait(const activity::ActivityImpl* act)
{
  if (const auto* comm = dynamic_cast<activity::CommImpl const*>(act)) {
    const std::string src_buff_id = ptr_to_id<unsigned char>(comm->src_buff_);
    const std::string dst_buff_id = ptr_to_id<unsigned char>(comm->dst_buff_);
    return "CommWait(comm_id:" + std::to_string(comm->get_id()) +
           " src:" + std::to_string(comm->src_actor_ != nullptr ? comm->src_actor_->get_pid() : -1) +
           " dst:" + std::to_string(comm->dst_actor_ != nullptr ? comm->dst_actor_->get_pid() : -1) +
           " mbox:" + (comm->get_mailbox() == nullptr ? "-" : comm->get_mailbox()->get_name()) +
           "(id:" + std::to_string(comm->get_mailbox_id()) + ") srcbuf:" + src_buff_id + " dstbuf:" + dst_buff_id +
           " bufsize:" + std::to_string(comm->src_buff_size_) + ")";
  } else {
    return "WaitUnknownType()";
  }
}

void ActivityWaitSimcall::serialize(std::stringstream& stream) const
{
  serialize_activity_wait(activity_, timeout_ > 0, stream);
}
void ActivityWaitanySimcall::serialize(std::stringstream& stream) const
{
  stream << (short)mc::Transition::Type::WAITANY << ' ' << activities_.size() << ' ';
  for (auto const* act : activities_) {
    serialize_activity_wait(act, timeout_ > 0, stream);
    stream << ' ';
  }
}
std::string ActivityWaitSimcall::to_string() const
{
  return to_string_activity_wait(activity_);
}
std::string ActivityWaitanySimcall::to_string() const
{
  std::stringstream buffer("WaitAny(");
  for (auto const* act : activities_) {
    buffer << to_string_activity_wait(act);
  }
  return buffer.str();
}
ActivityWaitanySimcall::ActivityWaitanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities,
                                               double timeout)
    : ResultingSimcall(actor, -1), activities_(activities), timeout_(timeout)
{
  // list all the activities that are ready
  indexes_.clear();
  for (unsigned i = 0; i < activities_.size(); i++)
    if (activities_[i]->test(get_issuer()))
      indexes_.push_back(i);
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

int ActivityWaitanySimcall::get_max_consider() const
{
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
  /* Note that the comm_ is 0 until after the execution of the simcall */
  stream << (short)mc::Transition::Type::COMM_ASYNC_SEND << ' ';
  stream << (comm_ ? comm_->get_id() : 0) << ' ' << mbox_->get_id() << ' ' << (uintptr_t)src_buff_ << ' '
         << src_buff_size_ << ' ' << tag_;
  XBT_DEBUG("SendObserver comm:%p mbox:%u buff:%p size:%zu tag:%d", comm_, mbox_->get_id(), src_buff_, src_buff_size_,
            tag_);
}
std::string CommIsendSimcall::to_string() const
{
  return "CommAsyncSend(comm_id: " + std::to_string(comm_->get_id()) + " mbox:" + std::to_string(mbox_->get_id()) +
         " srcbuf:" + ptr_to_id<unsigned char>(src_buff_) + " bufsize:" + std::to_string(src_buff_size_) +
         " tag: " + std::to_string(tag_) + ")";
}

void CommIrecvSimcall::serialize(std::stringstream& stream) const
{
  /* Note that the comm_ is 0 until after the execution of the simcall */
  stream << (short)mc::Transition::Type::COMM_ASYNC_RECV << ' ';
  stream << (comm_ ? comm_->get_id() : 0) << ' ' << mbox_->get_id() << ' ' << (uintptr_t)dst_buff_ << ' ' << tag_;
  XBT_DEBUG("RecvObserver comm:%p mbox:%u buff:%p tag:%d", comm_, mbox_->get_id(), dst_buff_, tag_);
}
std::string CommIrecvSimcall::to_string() const
{
  return "CommAsyncRecv(comm_id: " + std::to_string(comm_->get_id()) + " mbox:" + std::to_string(mbox_->get_id()) +
         " dstbuf:" + ptr_to_id<unsigned char>(dst_buff_) + " tag: " + std::to_string(tag_) + ")";
}

} // namespace simgrid::kernel::actor
