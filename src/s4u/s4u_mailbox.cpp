/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_network_private.h"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_channel,s4u,"S4U Communication Mailboxes");

namespace simgrid {
namespace s4u {

const char *Mailbox::name() {
  return pimpl_->name_;
}

MailboxPtr Mailbox::byName(const char*name)
{
  kernel::activity::MailboxImpl* mbox = kernel::activity::MailboxImpl::byNameOrNull(name);
  if (mbox == nullptr) {
    mbox = simix::kernelImmediate([name] {
      return kernel::activity::MailboxImpl::byNameOrCreate(name);
    });
  }
  return MailboxPtr(&mbox->piface_, true);
}

MailboxPtr Mailbox::byName(std::string name)
{
  return byName(name.c_str());
}

bool Mailbox::empty()
{
  return pimpl_->comm_queue.empty();
}

smx_activity_t Mailbox::front()
{
  return pimpl_->comm_queue.empty() ? nullptr : pimpl_->comm_queue.front();
}

void Mailbox::setReceiver(ActorPtr actor) {
  simix::kernelImmediate([this, actor]() {
    this->pimpl_->setReceiver(actor);
  });
}

/** @brief get the receiver (process associated to the mailbox) */
ActorPtr Mailbox::receiver() {
  if (pimpl_->permanent_receiver == nullptr)
    return ActorPtr();
  return pimpl_->permanent_receiver->iface();
}

}
}
