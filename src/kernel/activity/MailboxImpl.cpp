/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MailboxImpl.hpp"

#include "src/kernel/activity/CommImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_mailbox, simix, "Mailbox implementation");

static std::map<std::string, smx_mailbox_t>* mailboxes = new std::map<std::string, smx_mailbox_t>;

void SIMIX_mailbox_exit()
{
  for (auto const& elm : *mailboxes)
    delete elm.second;
  delete mailboxes;
}

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

namespace simgrid {
namespace kernel {
namespace activity {
/** @brief Returns the mailbox of that name, or nullptr */
MailboxImpl* MailboxImpl::byNameOrNull(const char* name)
{
  auto mbox = mailboxes->find(name);
  if (mbox != mailboxes->end())
    return mbox->second;
  else
    return nullptr;
}
/** @brief Returns the mailbox of that name, newly created on need */
MailboxImpl* MailboxImpl::byNameOrCreate(const char* name)
{
  xbt_assert(name, "Mailboxes must have a name");
  /* two processes may have pushed the same mbox_create simcall at the same time */
  auto m = mailboxes->find(name);
  if (m == mailboxes->end()) {
    smx_mailbox_t mbox = new MailboxImpl(name);
    XBT_DEBUG("Creating a mailbox at %p with name %s", mbox, name);
    (*mailboxes)[mbox->name_] = mbox;
    return mbox;
  } else
    return m->second;
}
/** @brief set the receiver of the mailbox to allow eager sends
 *  \param actor The receiving dude
 */
void MailboxImpl::setReceiver(s4u::ActorPtr actor)
{
  this->permanent_receiver = actor.get()->getImpl();
}
/** @brief Pushes a communication activity into a mailbox
 *  @param comm What to add
 */
void MailboxImpl::push(activity::CommImplPtr comm)
{
  comm->mbox = this;
  this->comm_queue.push_back(std::move(comm));
}

/** @brief Removes a communication activity from a mailbox
 *  @param activity What to remove
 */
void MailboxImpl::remove(smx_activity_t activity)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(activity);

  xbt_assert(comm->mbox == this, "Comm %p is in mailbox %s, not mailbox %s", comm.get(),
             (comm->mbox ? comm->mbox->getCname() : "(null)"), this->getCname());
  comm->mbox = nullptr;
  for (auto it = this->comm_queue.begin(); it != this->comm_queue.end(); it++)
    if (*it == comm) {
      this->comm_queue.erase(it);
      return;
    }
  xbt_die("Comm %p not found in mailbox %s", comm.get(), this->getCname());
}
}
}
}
