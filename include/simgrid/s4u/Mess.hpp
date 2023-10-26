/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MESS_HPP
#define SIMGRID_S4U_MESS_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>
#include <vector>

namespace simgrid::s4u {

class XBT_PUBLIC Mess : public Activity_T<Mess> {
#ifndef DOXYGEN
  friend MessageQueue; // Factory of messages
  friend kernel::activity::MessImpl;
#endif
  MessageQueue* queue_  = nullptr;
  void* payload_        = nullptr;
  size_t dst_buff_size_ = 0;
  void* dst_buff_       = nullptr;

  Mess() = default;
  Mess* do_start() override;

  static xbt::signal<void(Mess const&)> on_send;
  xbt::signal<void(Mess const&)> on_this_send;
  static xbt::signal<void(Mess const&)> on_recv;
  xbt::signal<void(Mess const&)> on_this_recv;

  /* These ensure that the on_completion signals are really thrown */
  void fire_on_completion_for_real() const { Activity_T<Mess>::fire_on_completion(); }
  void fire_on_this_completion_for_real() const { Activity_T<Mess>::fire_on_this_completion(); }

public:
#ifndef DOXYGEN
  Mess(Mess const&) = delete;
  Mess& operator=(Mess const&) = delete;
#endif

  MessPtr set_queue(MessageQueue* queue);
  MessageQueue* get_queue() const { return queue_; }

  /** Retrieve the payload associated to the communication. You can only do that once the comm is (gracefully)
   * terminated */
  void* get_payload() const { return payload_; }
  MessPtr set_payload(void* data);
  MessPtr set_dst_data(void** buff, size_t size);
  Actor* get_sender() const;
  Actor* get_receiver() const;

  bool is_assigned() const override { return true; };

  Mess* wait_for(double timeout) override;

  kernel::actor::ActorImpl* sender_   = nullptr;
  kernel::actor::ActorImpl* receiver_ = nullptr;
};
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MESS_HPP */
