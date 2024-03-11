/* Copyright (c) 2019-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MessageQueueImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_config.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(obs_comm, mc_observer, "Logging specific to the Communication simcalls observation");

namespace simgrid::kernel::actor {

void CommIsendSimcall::serialize(std::stringstream& stream) const
{
  /* Note that the comm_ is 0 until after the execution of the simcall */
  stream << (short)mc::Transition::Type::COMM_ASYNC_SEND << ' ';
  stream << (comm_ ? comm_->get_id() : 0) << ' ' << mbox_->get_id() << ' ' << tag_;
  XBT_DEBUG("SendObserver comm:%p mbox:%u tag:%d", comm_, mbox_->get_id(), tag_);
  stream << ' ' << fun_call_;
}
std::string CommIsendSimcall::to_string() const
{
  return "CommAsyncSend(comm_id: " + std::to_string(comm_ ? comm_->get_id() : 0) +
         " mbox:" + std::to_string(mbox_->get_id()) + " tag: " + std::to_string(tag_) + ")";
}

void CommIrecvSimcall::serialize(std::stringstream& stream) const
{
  /* Note that the comm_ is 0 until after the execution of the simcall */
  stream << (short)mc::Transition::Type::COMM_ASYNC_RECV << ' ';
  stream << (comm_ ? comm_->get_id() : 0) << ' ' << mbox_->get_id() << ' ' << tag_;
  XBT_DEBUG("RecvObserver comm:%p mbox:%u tag:%d", comm_, mbox_->get_id(), tag_);
  stream << ' ' << fun_call_;
}

std::string CommIrecvSimcall::to_string() const
{
  return "CommAsyncRecv(comm_id: " + std::to_string(comm_ ? comm_->get_id() : 0) +
         " mbox:" + std::to_string(mbox_->get_id()) + " tag: " + std::to_string(tag_) + ")";
}

void MessIputSimcall::serialize(std::stringstream& stream) const
{
  stream << mess_  << ' ' << queue_;
  XBT_DEBUG("PutObserver mess:%p queue:%p", mess_, queue_);
}

std::string MessIputSimcall::to_string() const
{
  return "MessAsyncPut(queue:" + queue_->get_name() + ")";
}

void MessIgetSimcall::serialize(std::stringstream& stream) const
{
  stream << mess_ << ' ' << queue_;
  XBT_DEBUG("GettObserver mess:%p queue:%p", mess_, queue_);
}

std::string MessIgetSimcall::to_string() const
{
  return "MessAsyncGet(queue:" + queue_->get_name() + ")";
}



} // namespace simgrid::kernel::actor
