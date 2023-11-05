/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP
#define SIMGRID_KERNEL_ACTIVITY_MAILBOX_HPP

#include "simgrid/config.h" /* FIXME: KILLME. This makes the ABI config-dependent, but mandatory for the hack below */
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

namespace simgrid::kernel::activity {

/** @brief Implementation of the s4u::Mailbox */

class MailboxImpl {
  s4u::Mailbox piface_;
  std::string name_;
  actor::ActorImplPtr permanent_receiver_; // actor to which the mailbox is attached

  std::deque<CommImplPtr> comm_queue_;
  // messages already received in the permanent receive mode
  std::deque<CommImplPtr> done_comm_queue_;

  friend s4u::Engine;
  friend s4u::Mailbox;
  friend s4u::Mailbox* s4u::Engine::mailbox_by_name_or_create(const std::string& name) const;
  friend s4u::Mailbox* s4u::Mailbox::by_name(const std::string& name);

  static unsigned next_id_; // Next ID to be given
  const unsigned id_ = next_id_++;
  explicit MailboxImpl(const std::string& name) : piface_(this), name_(name) {}
  MailboxImpl(const MailboxImpl&) = delete;
  MailboxImpl& operator=(const MailboxImpl&) = delete;

public:
  /** @brief Public interface */
  unsigned get_id() const { return id_; }

  ~MailboxImpl();

  const s4u::Mailbox* get_iface() const { return &piface_; }
  s4u::Mailbox* get_iface() { return &piface_; }

  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_receiver(s4u::ActorPtr actor);
  void push(const CommImplPtr& comm);
  void push_done(const CommImplPtr& done_comm) { done_comm_queue_.push_back(done_comm); }
  void remove(const CommImplPtr& comm);
  void clear(bool do_finish);
  CommImplPtr iprobe(int type, const std::function<bool(void*, void*, CommImpl*)>& match_fun, void* data);
  CommImplPtr find_matching_comm(CommImplType type, const std::function<bool(void*, void*, CommImpl*)>& match_fun,
                                 void* this_user_data, const CommImplPtr& my_synchro, bool done, bool remove_matching);
  bool is_permanent() const { return permanent_receiver_ != nullptr; }
  actor::ActorImplPtr get_permanent_receiver() const { return permanent_receiver_; }
  bool empty() const { return comm_queue_.empty(); }
  size_t size() const { return comm_queue_.size(); }
  const CommImplPtr& front() const { return comm_queue_.front(); }
  bool has_some_done_comm() const { return not done_comm_queue_.empty(); }
  const CommImplPtr& done_front() const { return done_comm_queue_.front(); }
};
} // namespace simgrid::kernel::activity

#endif
