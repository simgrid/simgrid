/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

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
  friend s4u::Mailbox;
  friend s4u::MailboxPtr s4u::Mailbox::by_name(std::string name);
  friend mc::CommunicationDeterminismChecker;

  explicit MailboxImpl(std::string name)
      : piface_(this), name_(name), comm_queue_(MAX_MAILBOX_SIZE), done_comm_queue_(MAX_MAILBOX_SIZE)
  {
  }

public:
  const simgrid::xbt::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  static MailboxImpl* by_name_or_null(std::string name);
  static MailboxImpl* by_name_or_create(std::string name);
  void set_receiver(s4u::ActorPtr actor);
  void push(activity::CommImplPtr comm);
  void remove(smx_activity_t activity);

private:
  simgrid::s4u::Mailbox piface_;
  simgrid::xbt::string name_;

public:
  simgrid::kernel::actor::ActorImplPtr permanent_receiver_; // actor to which the mailbox is attached
  boost::circular_buffer_space_optimized<smx_activity_t> comm_queue_;
  boost::circular_buffer_space_optimized<smx_activity_t>
      done_comm_queue_; // messages already received in the permanent receive mode
};
}
}
}

XBT_PRIVATE void SIMIX_mailbox_exit();

#endif /* SIMIX_MAILBOXIMPL_H */
