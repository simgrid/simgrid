/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MAILBOX_HPP
#define SIMGRID_S4U_MAILBOX_HPP

#include <xbt/base.h>
#include <simgrid/s4u/forward.hpp>

#ifdef __cplusplus

# include <boost/unordered_map.hpp>
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

private:
  Mailbox(const char*name, smx_mailbox_t inferior);
public:
  ~Mailbox();
  
protected:
  smx_mailbox_t getInferior() { return pimpl_; }

public:
  /** Get the name of that mailbox */
  const char *getName();
  /** Retrieve the mailbox associated to the given string */
  static Mailbox *byName(const char *name);
  /** Returns whether the mailbox contains queued communications */
  bool empty();

  /** Declare that the specified process is a permanent receiver on that mailbox
   *
   * It means that the communications sent to this mailbox will start flowing to its host even before he does a recv().
   * This models the real behavior of TCP and MPI communications, amongst other.
   */
  void setReceiver(smx_process_t process);
  /** Return the process declared as permanent receiver, or nullptr if none **/
  smx_process_t receiver();

private:
  std::string name_;
  smx_mailbox_t pimpl_;
  static boost::unordered_map<std::string, Mailbox *> *mailboxes;
  friend s4u::Engine;
};
}} // namespace simgrid::s4u

#endif /* C++ */

XBT_PUBLIC(sg_mbox_t) sg_mbox_by_name(const char*name);
XBT_PUBLIC(int) sg_mbox_is_empty(sg_mbox_t mbox);
XBT_PUBLIC(void)sg_mbox_setReceiver(sg_mbox_t mbox, smx_process_t process);
XBT_PUBLIC(smx_process_t) sg_mbox_receiver(sg_mbox_t mbox);

#endif /* SIMGRID_S4U_MAILBOX_HPP */
