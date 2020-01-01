/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP
#define SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP

#include <boost/circular_buffer.hpp>
#include <xbt/string.hpp>

#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

/** @brief Implementation of the s4u::Mailbox */

class MailboxImpl {
  static constexpr size_t MAX_MAILBOX_SIZE = 10000000;

  s4u::Mailbox piface_;
  xbt::string name_;

  friend s4u::Mailbox;
  friend s4u::Mailbox* s4u::Mailbox::by_name(const std::string& name);
  friend mc::CommunicationDeterminismChecker;

  explicit MailboxImpl(const std::string& name) : piface_(this), name_(name) {}

public:
  const xbt::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  static MailboxImpl* by_name_or_null(const std::string& name);
  static MailboxImpl* by_name_or_create(const std::string& name);
  void set_receiver(s4u::ActorPtr actor);
  void push(CommImplPtr comm);
  void remove(const CommImplPtr& comm);
  CommImplPtr iprobe(int type, bool (*match_fun)(void*, void*, CommImpl*), void* data);
  CommImplPtr find_matching_comm(CommImpl::Type type, bool (*match_fun)(void*, void*, CommImpl*), void* this_user_data,
                                 const CommImplPtr& my_synchro, bool done, bool remove_matching);

  actor::ActorImplPtr permanent_receiver_; // actor to which the mailbox is attached
  boost::circular_buffer_space_optimized<CommImplPtr> comm_queue_{MAX_MAILBOX_SIZE};
  // messages already received in the permanent receive mode
  boost::circular_buffer_space_optimized<CommImplPtr> done_comm_queue_{MAX_MAILBOX_SIZE};
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

XBT_PRIVATE void SIMIX_mailbox_exit();

#endif
