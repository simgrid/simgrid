/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MAILBOX_HPP
#define SIMGRID_S4U_MAILBOX_HPP

#include <string>

#include <boost/intrusive_ptr.hpp>

#include <xbt/base.h>

#include <simgrid/s4u/forward.hpp>
#include <simgrid/s4u/actor.hpp>

namespace simgrid {
namespace s4u {

/** @brief Mailboxes
 *
 * Rendez-vous point for network communications, similar to URLs on which you could post and retrieve data.
 * They are not network locations: you can post and retrieve on a given mailbox from anywhere on the network.
 * You can access any mailbox without any latency. The network delay are only related to the location of the
 * sender and receiver.
 */
XBT_PUBLIC_CLASS Mailbox {
  friend Comm;
  friend simgrid::s4u::Engine;
  friend simgrid::simix::Mailbox;

  smx_mailbox_t pimpl_;

  Mailbox(smx_mailbox_t mbox): pimpl_(mbox) {}

protected:
  smx_mailbox_t getInferior() { return pimpl_; }

public:

  // We don't have to manage the lifetime of mailboxes:
  friend void intrusive_ptr_add_ref(Mailbox*) {}
  friend void intrusive_ptr_release(Mailbox*) {}
  using Ptr = boost::intrusive_ptr<Mailbox>;

  /** Get the name of that mailbox */
  const char *getName();

  /** Retrieve the mailbox associated to the given string */
  static Ptr byName(const char *name);

  /** Returns whether the mailbox contains queued communications */
  bool empty();

  /** Declare that the specified process is a permanent receiver on that mailbox
   *
   * It means that the communications sent to this mailbox will start flowing to its host even before he does a recv().
   * This models the real behavior of TCP and MPI communications, amongst other.
   */
  void setReceiver(Actor* process);

  /** Return the process declared as permanent receiver, or nullptr if none **/
  Actor& receiver();
};

using MailboxPtr = Mailbox::Ptr;

}} // namespace simgrid::s4u

XBT_PUBLIC(sg_mbox_t) sg_mbox_by_name(const char*name);
XBT_PUBLIC(int) sg_mbox_is_empty(sg_mbox_t mbox);
XBT_PUBLIC(void)sg_mbox_setReceiver(sg_mbox_t mbox, smx_process_t process);
XBT_PUBLIC(smx_process_t) sg_mbox_receiver(sg_mbox_t mbox);

#endif /* SIMGRID_S4U_MAILBOX_HPP */
