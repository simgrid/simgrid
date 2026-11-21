/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/forward.h"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/internal_config.h"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MessageQueueImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/remote/Channel.hpp"
#include "src/mc/transition/Transition.hpp"
#include <string>

#if HAVE_SMPI
#include "smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(obs_comm, mc_observer, "Logging specific to the Communication simcalls observation");

namespace simgrid::kernel::actor {

void CommIsendSimcall::serialize(mc::Channel& channel) const
{
  /* Note that the comm_ is 0 until after the execution of the simcall */
  channel.pack(mc::Transition::Type::COMM_ASYNC_SEND);
  channel.pack<unsigned>((comm_ ? comm_->get_id() : 0));
  channel.pack<unsigned>(mbox_->get_id());
  channel.pack<int>(tag_);
  channel.pack<std::string>(fun_call_);
  XBT_DEBUG("SendObserver comm:%p mbox:%u tag:%d", comm_, mbox_->get_id(), tag_);
}
std::string CommIsendSimcall::to_string() const
{
  return "CommAsyncSend(comm_id: " + std::to_string(comm_ ? comm_->get_id() : 0) +
         " mbox:" + std::to_string(mbox_->get_id()) + " tag: " + std::to_string(tag_) + ")";
}

void CommIrecvSimcall::serialize(mc::Channel& channel) const
{
  /* Note that the comm_ is 0 until after the execution of the simcall */
  channel.pack(mc::Transition::Type::COMM_ASYNC_RECV);
  channel.pack<unsigned>((comm_ ? comm_->get_id() : 0));
  channel.pack<unsigned>(mbox_->get_id());
  channel.pack<int>(tag_);
  channel.pack<std::string>(fun_call_);
  XBT_DEBUG("RecvObserver comm:%p mbox:%u tag:%d", comm_, mbox_->get_id(), tag_);
}

std::string CommIrecvSimcall::to_string() const
{
  return "CommAsyncRecv(comm_id: " + std::to_string(comm_ ? comm_->get_id() : 0) +
         " mbox:" + std::to_string(mbox_->get_id()) + " tag: " + std::to_string(tag_) + ")";
}
ProbeSimcall::ProbeSimcall(ActorImpl* actor, activity::MailboxImpl* mbox, s4u::Mailbox::IprobeKind kind,
                           const std::function<bool(void*, void*, activity::CommImpl*)>& match_fun, void* match_data,
                           bool blocking)
    : DelayedSimcallObserver(actor, nullptr)
    , mbox_(mbox)
    , kind_(kind)
    , blocking_(blocking)
    , match_fun_(match_fun)
    , match_data_(match_data)
{
#if HAVE_SMPI
  auto req = static_cast<smpi::Request*>(match_data);
  tag_     = req->tag();
  any_tag_ = tag_ == MPI_ANY_TAG;
  src_     = req->src() == MPI_ANY_SOURCE ? -1 : req->src();
#endif
}

bool ProbeSimcall::is_enabled()
{
  return not blocking_ || mbox_->iprobe(kind_, match_fun_, match_data_) != nullptr;
}

void ProbeSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(blocking_ ? mc::Transition::Type::COMM_PROBE : mc::Transition::Type::COMM_IPROBE);
  channel.pack<unsigned>(mbox_->get_id());
  channel.pack<bool>((kind_ == s4u::Mailbox::IprobeKind::SEND));
  channel.pack<int>(tag_);
  channel.pack<bool>(any_tag_);
  channel.pack<aid_t>(src_);
  XBT_DEBUG("ProbeObserver blocking:%d mbox:%u kind:%s tag:%d any_tag:%d src:%ld", blocking_, mbox_->get_id(),
            (kind_ == s4u::Mailbox::IprobeKind::SEND ? "send" : "recv"), tag_, any_tag_, src_);
}
std::string ProbeSimcall::to_string() const
{
  return std::string(blocking_ ? "Probe" : "Iprobe") + "(mbox:" + std::to_string(mbox_->get_id()) +
         " tag: " + std::to_string(tag_) + " kind:" + (kind_ == s4u::Mailbox::IprobeKind::SEND ? "send" : "recv") + ")";
}

void MessIputSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::COMM_ASYNC_SEND);
  channel.pack(mess_);
  channel.pack(queue_);
  XBT_DEBUG("PutObserver mess:%p queue:%p", mess_, queue_);
}

std::string MessIputSimcall::to_string() const
{
  return "MessAsyncPut(queue:" + queue_->get_name() + ")";
}

void MessIgetSimcall::serialize(mc::Channel& channel) const
{
  channel.pack(mc::Transition::Type::COMM_ASYNC_RECV);
  channel.pack(mess_);
  channel.pack(queue_);
  XBT_DEBUG("GettObserver mess:%p queue:%p", mess_, queue_);
}

std::string MessIgetSimcall::to_string() const
{
  return "MessAsyncGet(queue:" + queue_->get_name() + ")";
}



} // namespace simgrid::kernel::actor
