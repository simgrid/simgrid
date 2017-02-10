/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_MAILBOXIMPL_H
#define SIMIX_MAILBOXIMPL_H

#include <boost/circular_buffer.hpp>

#include "simgrid/s4u/Mailbox.hpp"
#include "src/simix/ActorImpl.hpp"

#define MAX_MAILBOX_SIZE 10000000
namespace simgrid {
namespace simix {

/** @brief Rendez-vous point datatype */

class MailboxImpl {
public:
  explicit MailboxImpl(const char* name)
      : piface_(this), name_(xbt_strdup(name)), comm_queue(MAX_MAILBOX_SIZE), done_comm_queue(MAX_MAILBOX_SIZE)
  {
  }
  ~MailboxImpl() { xbt_free(name_); }

  simgrid::s4u::Mailbox piface_; // Our interface
  char* name_;
  boost::circular_buffer_space_optimized<smx_activity_t> comm_queue;
  boost::intrusive_ptr<simgrid::simix::ActorImpl> permanent_receiver; //process which the mailbox is attached to
  boost::circular_buffer_space_optimized<smx_activity_t> done_comm_queue;//messages already received in the permanent receive mode
};
}
}

XBT_PRIVATE void SIMIX_mailbox_exit();

XBT_PRIVATE smx_mailbox_t SIMIX_mbox_create(const char* name);
XBT_PRIVATE smx_mailbox_t SIMIX_mbox_get_by_name(const char* name);
XBT_PRIVATE void SIMIX_mbox_remove(smx_mailbox_t mbox, smx_activity_t comm);
XBT_PRIVATE void SIMIX_mbox_push(smx_mailbox_t mbox, smx_activity_t synchro);

XBT_PRIVATE void SIMIX_mbox_set_receiver(smx_mailbox_t mbox, smx_actor_t proc);

#endif /* SIMIX_MAILBOXIMPL_H */
