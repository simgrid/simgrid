/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MESS_HPP
#define SIMGRID_KERNEL_ACTIVITY_MESS_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/CommObserver.hpp"

namespace simgrid::kernel::activity {

enum class MessImplType { PUT, GET };

class XBT_PUBLIC MessImpl : public ActivityImpl_T<MessImpl> {
  ~MessImpl() override;

  MessageQueueImpl* queue_ = nullptr;
  void* payload_           = nullptr;
  MessImplType type_       = MessImplType::PUT;
  unsigned char* dst_buff_ = nullptr;
  size_t* dst_buff_size_   = nullptr;

public:
  MessImpl& set_type(MessImplType type);
  MessImplType get_type() const { return type_; }
  MessImpl& set_payload(void* payload);
  void* get_payload() { return payload_; }

  MessImpl& set_queue(MessageQueueImpl* queue);
  MessageQueueImpl* get_queue() const { return queue_; }
  MessImpl& set_dst_buff(unsigned char* buff, size_t* size);

  static ActivityImplPtr iput(actor::MessIputSimcall* observer);
  static ActivityImplPtr iget(actor::MessIgetSimcall* observer);

  void wait_for(actor::ActorImpl* issuer, double timeout) override;

  MessImpl* start();
  void suspend() override { /* no action to suspend for Mess */ }
  void resume() override { /* no action to resume for Mess */ }
  void cancel() override;
  void set_exception(actor::ActorImpl* issuer) override {};
  void finish() override;

  actor::ActorImplPtr src_actor_ = nullptr;
  actor::ActorImplPtr dst_actor_ = nullptr;
};
} // namespace simgrid::kernel::activity

#endif
