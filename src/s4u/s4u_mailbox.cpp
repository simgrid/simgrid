/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_network_private.h"
#include "src/simix/smx_process_private.h"

#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_channel,s4u,"S4U Communication Mailboxes");

namespace simgrid {
namespace s4u {

const char *Mailbox::getName() {
  return pimpl_->name;
}

MailboxPtr Mailbox::byName(const char*name) {
  // FIXME: there is a race condition here where two actors run Mailbox::byName
  // on a non-existent mailbox during the same scheduling round. Both will be
  // interrupted in the simcall creating the underlying simix mbox.
  // Only one simix object will be created, but two S4U objects will be created.
  // Only one S4U object will be stored in the hashmap and used, and the other
  // one will be leaked.
  smx_mailbox_t mbox = simcall_mbox_get_by_name(name);
  if (mbox == nullptr)
    mbox = simcall_mbox_create(name);
  return MailboxPtr(&mbox->piface_, true);
}

bool Mailbox::empty() {
  return nullptr == simcall_mbox_front(pimpl_);
}

void Mailbox::setReceiver(Actor* actor) {
  simcall_mbox_set_receiver(pimpl_, actor == nullptr ? nullptr : actor->pimpl_);
}

/** @brief get the receiver (process associated to the mailbox) */
ActorPtr Mailbox::receiver() {
  if(pimpl_->permanent_receiver == nullptr) return ActorPtr();
  return ActorPtr(&pimpl_->permanent_receiver->actor());
}

}
}

/*------- C functions -------*/

sg_mbox_t sg_mbox_by_name(const char*name){
  return simgrid::s4u::Mailbox::byName(name).get();
}
int sg_mbox_is_empty(sg_mbox_t mbox) {
  return mbox->empty();
}
void sg_mbox_setReceiver(sg_mbox_t mbox, smx_process_t process) {
  mbox->setReceiver(&process->actor());
}
