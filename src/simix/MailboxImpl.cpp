/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/MailboxImpl.hpp"
#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_mailbox, simix, "Mailbox implementation");

static void SIMIX_mbox_free(void* data);
static xbt_dict_t mailboxes = xbt_dict_new_homogeneous(SIMIX_mbox_free);

void SIMIX_mailbox_exit()
{
  xbt_dict_free(&mailboxes);
}
void SIMIX_mbox_free(void* data)
{
  XBT_DEBUG("mbox free %p", data);
  smx_mailbox_t mbox = static_cast<smx_mailbox_t>(data);
  delete mbox;
}

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

/**
 *  \brief set the receiver of the rendez vous point to allow eager sends
 *  \param mbox The rendez-vous point
 *  \param process The receiving process
 */
void SIMIX_mbox_set_receiver(smx_mailbox_t mbox, smx_actor_t process)
{
  mbox->permanent_receiver = process;
}

namespace simgrid {
namespace simix {
/** @brief Returns the mailbox of that name, or nullptr */
MailboxImpl* MailboxImpl::byNameOrNull(const char* name)
{
  return static_cast<smx_mailbox_t>(xbt_dict_get_or_null(mailboxes, name));
}
/** @brief Returns the mailbox of that name, newly created on need */
MailboxImpl* MailboxImpl::byNameOrCreate(const char* name)
{
  xbt_assert(name, "Mailboxes must have a name");
  /* two processes may have pushed the same mbox_create simcall at the same time */
  smx_mailbox_t mbox = static_cast<smx_mailbox_t>(xbt_dict_get_or_null(mailboxes, name));
  if (!mbox) {
    mbox = new simgrid::simix::MailboxImpl(name);
    XBT_DEBUG("Creating a mailbox at %p with name %s", mbox, name);
    xbt_dict_set(mailboxes, mbox->name_, mbox, nullptr);
  }
  return mbox;
}
/** @brief Pushes a communication activity into a mailbox
 *  @param activity What to add
 */
void MailboxImpl::push(smx_activity_t synchro)
{
  simgrid::kernel::activity::Comm* comm = static_cast<simgrid::kernel::activity::Comm*>(synchro);
  this->comm_queue.push_back(comm);
  comm->mbox = this;
}

/** @brief Removes a communication activity from a mailbox
 *  @param activity What to remove
 */
void MailboxImpl::remove(smx_activity_t activity)
{
  simgrid::kernel::activity::Comm* comm = static_cast<simgrid::kernel::activity::Comm*>(activity);

  comm->mbox = nullptr;
  for (auto it = this->comm_queue.begin(); it != this->comm_queue.end(); it++)
    if (*it == comm) {
      this->comm_queue.erase(it);
      return;
    }
  xbt_die("Cannot remove the comm %p that is not part of the mailbox %s", comm, this->name_);
}
}
}
