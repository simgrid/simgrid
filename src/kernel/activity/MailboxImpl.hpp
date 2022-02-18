/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP
#define SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP

#include <boost/circular_buffer.hpp>
#include <xbt/string.hpp>

#include "simgrid/s4u/Engine.hpp"
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
  actor::ActorImplPtr permanent_receiver_; // actor to which the mailbox is attached
  boost::circular_buffer_space_optimized<CommImplPtr> comm_queue_{MAX_MAILBOX_SIZE};
  // messages already received in the permanent receive mode
  boost::circular_buffer_space_optimized<CommImplPtr> done_comm_queue_{MAX_MAILBOX_SIZE};

  friend s4u::Engine;
  friend s4u::Mailbox;
  friend s4u::Mailbox* s4u::Engine::mailbox_by_name_or_create(const std::string& name) const;
  friend s4u::Mailbox* s4u::Mailbox::by_name(const std::string& name);

  static unsigned next_id_; // Next ID to be given
  const unsigned id_ = next_id_++;
  explicit MailboxImpl(const std::string& name) : piface_(this), name_(name) {}

public:
  /** @brief Public interface */
  unsigned get_id() const { return id_; }

  const s4u::Mailbox* get_iface() const { return &piface_; }
  s4u::Mailbox* get_iface() { return &piface_; }

  const xbt::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_receiver(s4u::ActorPtr actor);
  void push(CommImplPtr comm);
  void push_done(CommImplPtr done_comm) { done_comm_queue_.push_back(done_comm); }
  void remove(const CommImplPtr& comm);
  CommImplPtr iprobe(int type, bool (*match_fun)(void*, void*, CommImpl*), void* data);
  CommImplPtr find_matching_comm(CommImpl::Type type, bool (*match_fun)(void*, void*, CommImpl*), void* this_user_data,
                                 const CommImplPtr& my_synchro, bool done, bool remove_matching);
  bool is_permanent() const { return permanent_receiver_ != nullptr; }
  actor::ActorImplPtr get_permanent_receiver() const { return permanent_receiver_; }
  bool empty() const { return comm_queue_.empty(); }
  size_t size() const { return comm_queue_.size(); }
  CommImplPtr front() const { return comm_queue_.front(); }
  bool has_some_done_comm() const { return not done_comm_queue_.empty(); }
  CommImplPtr done_front() const { return done_comm_queue_.front(); }
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
