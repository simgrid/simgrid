/* Mailboxes in MSG */

/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "simgrid/msg.h"
#include "msg_private.h"
#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_mailbox, msg, "Logging specific to MSG (mailbox)");

/** \ingroup msg_mailbox_management
 * \brief Set the mailbox to receive in asynchronous mode
 *
 * All messages sent to this mailbox will be transferred to the receiver without waiting for the receive call.
 * The receive call will still be necessary to use the received data.
 * If there is a need to receive some messages asynchronously, and some not, two different mailboxes should be used.
 *
 * \param alias The name of the mailbox
 */
void MSG_mailbox_set_async(const char *alias){
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(alias);

  simcall_mbox_set_receiver(mailbox->getImpl(), SIMIX_process_self());
  XBT_VERB("%s mailbox set to receive eagerly for myself\n",alias);
}

/** \ingroup msg_mailbox_management
 * \brief Get a task from a mailbox on a given host at a given rate
 *
 * \param mailbox The mailbox where the task was sent
 * \param task a memory location for storing a #msg_task_t.
 * \param host a #msg_host_t host from where the task was sent
 * \param timeout a timeout
 * \param rate a rate

 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_mailbox_get_task_ext_bounded(msg_mailbox_t mailbox, msg_task_t * task, msg_host_t host, double timeout,
                                             double rate)
{
  msg_error_t ret = MSG_OK;
  /* We no longer support getting a task from a specific host */
  if (host)
    THROW_UNIMPLEMENTED;

  TRACE_msg_task_get_start();
  double start_time = MSG_get_clock();

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_WARN("Asked to write the received task in a non empty struct -- proceeding.");

  /* Try to receive it by calling SIMIX network layer */
  try {
    simcall_comm_recv(MSG_process_self(), mailbox->getImpl(), task, nullptr, nullptr, nullptr, nullptr, timeout, rate);
    XBT_DEBUG("Got task %s from %s",(*task)->name,mailbox->getName());
    (*task)->simdata->setNotUsed();
  }
  catch (xbt_ex& e) {
    switch (e.category) {
    case cancel_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    case host_error:
      ret = MSG_HOST_FAILURE;
      break;
    default:
      throw;
    }
  }

  if (ret != MSG_HOST_FAILURE && ret != MSG_TRANSFER_FAILURE && ret != MSG_TIMEOUT) {
    TRACE_msg_task_get_end(start_time, *task);
  }
  return ret;
}
