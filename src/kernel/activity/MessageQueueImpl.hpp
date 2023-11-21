/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MESSAGEQUEUE_HPP
#define SIMGRID_KERNEL_ACTIVITY_MESSAGEQUEUE_HPP

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/MessageQueue.hpp"
#include "src/kernel/activity/MessImpl.hpp"

namespace simgrid::kernel::activity {

/** @brief Implementation of the s4u::MessageQueue */

class MessageQueueImpl {
  s4u::MessageQueue piface_;
  std::string name_;
  std::deque<MessImplPtr> queue_;

  friend s4u::Engine;
  friend s4u::MessageQueue;
  friend s4u::MessageQueue* s4u::Engine::message_queue_by_name_or_create(const std::string& name) const;
  friend s4u::MessageQueue* s4u::MessageQueue::by_name(const std::string& name);

  static unsigned next_id_; // Next ID to be given
  const unsigned id_ = next_id_++;
  explicit MessageQueueImpl(const std::string& name) : piface_(this), name_(name) {}
  MessageQueueImpl(const MailboxImpl&) = delete;
  MessageQueueImpl& operator=(const MailboxImpl&) = delete;

public:
  ~MessageQueueImpl();

  /** @brief Public interface */
  unsigned get_id() const { return id_; }

  const s4u::MessageQueue* get_iface() const { return &piface_; }
  s4u::MessageQueue* get_iface() { return &piface_; }

  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void push(const MessImplPtr& mess);
  void remove(const MessImplPtr& mess);
  void clear();
  bool empty() const { return queue_.empty(); }
  size_t size() const { return queue_.size(); }
  const MessImplPtr& front() const { return queue_.front(); }

  MessImplPtr find_matching_message(MessImplType type);
};
} // namespace simgrid::kernel::activity

#endif
