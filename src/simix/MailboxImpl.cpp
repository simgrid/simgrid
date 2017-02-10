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

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

smx_mailbox_t SIMIX_mbox_create(const char* name)
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

void SIMIX_mbox_free(void* data)
{
  XBT_DEBUG("mbox free %p", data);
  smx_mailbox_t mbox = static_cast<smx_mailbox_t>(data);
  delete mbox;
}

smx_mailbox_t SIMIX_mbox_get_by_name(const char* name)
{
  return static_cast<smx_mailbox_t>(xbt_dict_get_or_null(mailboxes, name));
}

/**
 *  \brief set the receiver of the rendez vous point to allow eager sends
 *  \param mbox The rendez-vous point
 *  \param process The receiving process
 */
void SIMIX_mbox_set_receiver(smx_mailbox_t mbox, smx_actor_t process)
{
  mbox->permanent_receiver = process;
}

/**
 *  \brief Pushes a communication synchro into a rendez-vous point
 *  \param mbox The mailbox
 *  \param synchro The communication synchro
 */
void SIMIX_mbox_push(smx_mailbox_t mbox, smx_activity_t synchro)
{
  simgrid::kernel::activity::Comm* comm = static_cast<simgrid::kernel::activity::Comm*>(synchro);
  mbox->comm_queue.push_back(comm);
  comm->mbox = mbox;
}

/**
 *  \brief Removes a communication synchro from a rendez-vous point
 *  \param mbox The rendez-vous point
 *  \param synchro The communication synchro
 */
void SIMIX_mbox_remove(smx_mailbox_t mbox, smx_activity_t synchro)
{
  simgrid::kernel::activity::Comm* comm = static_cast<simgrid::kernel::activity::Comm*>(synchro);

  comm->mbox = nullptr;
  for (auto it = mbox->comm_queue.begin(); it != mbox->comm_queue.end(); it++)
    if (*it == comm) {
      mbox->comm_queue.erase(it);
      return;
    }
  xbt_die("Cannot remove the comm %p that is not part of the mailbox %s", comm, mbox->name_);
}
