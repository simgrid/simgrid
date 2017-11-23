/* Mailboxes in MSG */

/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Mailbox.hpp"
#include "src/msg/msg_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_mailbox, msg, "Logging specific to MSG (mailbox)");

extern "C" {

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
  mailbox->setReceiver(simgrid::s4u::Actor::self());
  XBT_VERB("%s mailbox set to receive eagerly for myself\n",alias);
}
}
