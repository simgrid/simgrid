/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_network_private.hpp"
#include "xbt/log.h"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_channel, s4u, "S4U Communication Mailboxes");

namespace simgrid {
namespace s4u {

const simgrid::xbt::string& Mailbox::get_name() const
{
  return pimpl_->get_name();
}

const char* Mailbox::get_cname() const
{
  return pimpl_->get_cname();
}

MailboxPtr Mailbox::by_name(const char* name)
{
  kernel::activity::MailboxImpl* mbox = kernel::activity::MailboxImpl::byNameOrNull(name);
  if (mbox == nullptr) {
    mbox = simix::simcall([name] { return kernel::activity::MailboxImpl::byNameOrCreate(name); });
  }
  return MailboxPtr(&mbox->piface_, true);
}

MailboxPtr Mailbox::by_name(std::string name)
{
  return by_name(name.c_str());
}

bool Mailbox::empty()
{
  return pimpl_->comm_queue.empty();
}

bool Mailbox::listen()
{
  return not this->empty() || (pimpl_->permanent_receiver && not pimpl_->done_comm_queue.empty());
}

smx_activity_t Mailbox::front()
{
  return pimpl_->comm_queue.empty() ? nullptr : pimpl_->comm_queue.front();
}

void Mailbox::set_receiver(ActorPtr actor)
{
  simix::simcall([this, actor]() { this->pimpl_->setReceiver(actor); });
}

/** @brief get the receiver (process associated to the mailbox) */
ActorPtr Mailbox::get_receiver()
{
  if (pimpl_->permanent_receiver == nullptr)
    return ActorPtr();
  return pimpl_->permanent_receiver->iface();
}

CommPtr Mailbox::put_init()
{
  CommPtr res   = CommPtr(new s4u::Comm());
  res->sender_  = SIMIX_process_self();
  res->mailbox_ = this;
  return res;
}
s4u::CommPtr Mailbox::put_init(void* data, uint64_t simulatedSize)
{
  s4u::CommPtr res = put_init();
  res->set_remaining(simulatedSize);
  res->src_buff_      = data;
  res->src_buff_size_ = sizeof(void*);
  return res;
}
s4u::CommPtr Mailbox::put_async(void* payload, uint64_t simulatedSize)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");

  s4u::CommPtr res = put_init(payload, simulatedSize);
  res->start();
  return res;
}
void Mailbox::put(void* payload, uint64_t simulatedSize)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");

  CommPtr c = put_init();
  c->set_remaining(simulatedSize);
  c->set_src_data(payload);
  c->wait();
}
/** Blocking send with timeout */
void Mailbox::put(void* payload, uint64_t simulatedSize, double timeout)
{
  xbt_assert(payload != nullptr, "You cannot send nullptr");

  CommPtr c = put_init();
  c->set_remaining(simulatedSize);
  c->set_src_data(payload);
  // c->start() is optional.
  c->wait(timeout);
}

s4u::CommPtr Mailbox::get_init()
{
  CommPtr res    = CommPtr(new s4u::Comm());
  res->receiver_ = SIMIX_process_self();
  res->mailbox_  = this;
  return res;
}
s4u::CommPtr Mailbox::get_async(void** data)
{
  s4u::CommPtr res = get_init();
  res->set_dst_data(data, sizeof(*data));
  res->start();
  return res;
}

void* Mailbox::get()
{
  void* res = nullptr;
  CommPtr c = get_init();
  c->set_dst_data(&res, sizeof(res));
  c->wait();
  return res;
}
void* Mailbox::get(double timeout)
{
  void* res = nullptr;
  CommPtr c = get_init();
  c->set_dst_data(&res, sizeof(res));
  c->wait(timeout);
  return res;
}
} // namespace s4u
} // namespace simgrid
