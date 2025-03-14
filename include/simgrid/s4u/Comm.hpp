/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>
#include <vector>

namespace simgrid::s4u {
/** @brief Communication async
 *
 * Represents all asynchronous communications, that you can test or wait onto.
 */
class XBT_PUBLIC Comm : public Activity_T<Comm> {
  friend Mailbox; // Factory of comms
  friend kernel::activity::CommImpl;
  /* specified for normal mailbox-based communications*/
  Mailbox* mailbox_                   = nullptr;
  kernel::actor::ActorImpl* sender_   = nullptr;
  kernel::actor::ActorImpl* receiver_ = nullptr;
  double rate_                        = -1;
  void* dst_buff_                     = nullptr;
  size_t dst_buff_size_               = 0;
  void* src_buff_                     = nullptr;
  size_t src_buff_size_               = sizeof(void*);

  /* FIXME: expose these elements in the API */
  std::function<void(kernel::activity::CommImpl*, void*, size_t)> copy_data_function_;

  Comm() = default;
  Comm* do_start() override;

  static xbt::signal<void(Comm const&)> on_send;
  xbt::signal<void(Comm const&)> on_this_send;
  static xbt::signal<void(Comm const&)> on_recv;
  xbt::signal<void(Comm const&)> on_this_recv;

protected:
  void fire_on_completion() const override {
    /* The completion signal of a Comm has to be thrown only once and not by the sender AND the receiver.
       then Comm::on_completion is thrown in the kernel in CommImpl::finish.
     */
  }
  void fire_on_this_completion() const override {
    /* The completion signal of a Comm has to be thrown only once and not by the sender AND the receiver.
       then Comm::on_this_completion is thrown in the kernel in CommImpl::finish.
     */
  }
  /* These ensure that the on_completion signals are really thrown */
  void fire_on_completion_for_real() const;
  void fire_on_this_completion_for_real() const;

public:
  /*! \static Add a callback fired when the send of any Comm is posted  */
  static void on_send_cb(const std::function<void(Comm const&)>& cb) { on_send.connect(cb); }
  /*! Add a callback fired when the send of this specific Comm is posted  */
  void on_this_send_cb(const std::function<void(Comm const&)>& cb) { on_this_send.connect(cb); }
  /*! \static Add a callback fired when the recv of any Comm is posted  */
  static void on_recv_cb(const std::function<void(Comm const&)>& cb) { on_recv.connect(cb); }
  /*! Add a callback fired when the recv of this specific Comm is posted  */
  void on_this_recv_cb(const std::function<void(Comm const&)>& cb) { on_this_recv.connect(cb); }

  CommPtr set_copy_data_callback(const std::function<void(kernel::activity::CommImpl*, void*, size_t)>& callback);

  ~Comm() override;

  static void send(kernel::actor::ActorImpl* sender, const s4u::Mailbox* mbox, double task_size, double rate,
                   void* src_buff, size_t src_buff_size,
                   const std::function<bool(void*, void*, simgrid::kernel::activity::CommImpl*)>& match_fun,
                   const std::function<void(simgrid::kernel::activity::CommImpl*, void*, size_t)>& copy_data_fun,
                   void* data, double timeout);
  static void recv(kernel::actor::ActorImpl* receiver, const s4u::Mailbox* mbox, void* dst_buff, size_t* dst_buff_size,
                   const std::function<bool(void*, void*, simgrid::kernel::activity::CommImpl*)>& match_fun,
                   const std::function<void(simgrid::kernel::activity::CommImpl*, void*, size_t)>& copy_data_fun,
                   void* data, double timeout, double rate);

  /* \static
   * "One-sided" communications. This way of communicating bypasses the mailbox and actors mechanism. It creates a
   * communication (vetoabled, asynchronous, or synchronous) directly between two hosts. There is really no limit on
   * the hosts involved. In particular, the actor creating such a communication does not have to be on one of the
   * involved hosts! Enjoy the comfort of the simulator :)
   */
  static CommPtr sendto_init(); /* Source and Destination hosts have to be set before the communication can start */
  static CommPtr sendto_init(s4u::Host* from, s4u::Host* to);
  static CommPtr sendto_async(s4u::Host* from, s4u::Host* to, uint64_t simulated_size_in_bytes);
  static void sendto(s4u::Host* from, s4u::Host* to, uint64_t simulated_size_in_bytes);

  CommPtr set_source(s4u::Host* from);
  s4u::Host* get_source() const;
  CommPtr set_destination(s4u::Host* to);
  s4u::Host* get_destination() const;

  /* Mailbox-based communications */
  CommPtr set_mailbox(s4u::Mailbox* mailbox);
  /** Retrieve the mailbox on which this comm acts. This is nullptr if the comm was created through sendto() */
  s4u::Mailbox* get_mailbox() const { return mailbox_; }

  /** Specify the data to send.
   *
   * @beginrst
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given by :cpp:func:`simgrid::s4u::Comm::set_payload_size`):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   * @endrst
   */
  CommPtr set_src_data(void* buff);
  /** Specify the size of the data to send (not to be mixed with set_payload_size())
   *
   * @beginrst
   * That's the size of the data to actually copy in the simulator (ie, the data passed with
   * :cpp:func:`simgrid::s4u::Comm::set_src_data`). That's completely unrelated from the simulated size (given by
   * :cpp:func:`simgrid::s4u::Comm::set_payload_size`)): you can send a short buffer in your simulator, that represents
   * a very large message in the simulated world, or the opposite.
   * @endrst
   */
  CommPtr set_src_data_size(size_t size);
  /** Specify the data to send and its size (not to be mixed with set_payload_size())
   *
   * @beginrst
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given by :cpp:func:`simgrid::s4u::Comm::set_payload_size`):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   * @endrst
   */
  CommPtr set_src_data(void* buff, size_t size);

  /** Specify where to receive the data.
   *
   * That's a buffer where the sent data will be copied */
  CommPtr set_dst_data(void** buff);
  /** Specify the buffer in which the data should be received
   *
   * That's a buffer where the sent data will be copied  */
  CommPtr set_dst_data(void** buff, size_t size);
  /** Retrieve where the data will be copied on the receiver side */
  void* get_dst_data() const { return dst_buff_; }
  /** Retrieve the size of the received data. Not to be mixed with @ref Activity::get_remaining()  */
  size_t get_dst_data_size() const { return dst_buff_size_; }
  /** Retrieve the payload associated to the communication. You can only do that once the comm is (gracefully)
   * terminated. */
  void* get_payload() const;

  /* Common functions */

  /** Specify the amount of bytes which exchange should be simulated (not to be mixed with set_src_data_size())
   *
   * @beginrst
   * That's the size of the simulated data, that's completely unrelated from the actual data size (given by
   * :cpp:func:`simgrid::s4u::Comm::set_src_data_size`).
   * @endrst
   */
  CommPtr set_payload_size(uint64_t bytes);
  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  CommPtr set_rate(double rate);

  bool is_assigned() const override;
  Actor* get_sender() const;
  Actor* get_receiver() const;

  /* Comm life cycle */
  Comm* wait_for(double timeout) override;
};
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COMM_HPP */
