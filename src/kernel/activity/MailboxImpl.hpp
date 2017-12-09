/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_MAILBOXIMPL_H
#define SIMIX_MAILBOXIMPL_H

#include <boost/circular_buffer.hpp>
#include <xbt/string.hpp>

#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/simix/ActorImpl.hpp"

#define MAX_MAILBOX_SIZE 10000000
namespace simgrid {
namespace kernel {
namespace activity {

/** @brief Implementation of the simgrid::s4u::Mailbox */

class MailboxImpl {
  explicit MailboxImpl(const char* name)
      : piface_(this), name_(name), comm_queue(MAX_MAILBOX_SIZE), done_comm_queue(MAX_MAILBOX_SIZE)
  {
  }

public:
  const simgrid::xbt::string& getName() const { return name_; }
  const char* getCname() const { return name_.c_str(); }
  static MailboxImpl* byNameOrNull(const char* name);
  static MailboxImpl* byNameOrCreate(const char* name);
  void setReceiver(s4u::ActorPtr actor);
  void push(activity::CommImplPtr comm);
  void remove(smx_activity_t activity);
  simgrid::s4u::Mailbox piface_; // Our interface
  simgrid::xbt::string name_;

  simgrid::simix::ActorImplPtr permanent_receiver; // process to which the mailbox is attached
  boost::circular_buffer_space_optimized<smx_activity_t> comm_queue;
  boost::circular_buffer_space_optimized<smx_activity_t> done_comm_queue; // messages already received in the permanent receive mode
};
}
}
}

XBT_PRIVATE void SIMIX_mailbox_exit();

#endif /* SIMIX_MAILBOXIMPL_H */
