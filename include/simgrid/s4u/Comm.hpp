/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

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
  bool detached_                                                          = false;
  std::function<bool(void*, void*, kernel::activity::CommImpl*)> match_fun_;
  std::function<void(void*)> clean_fun_;
  std::function<void(kernel::activity::CommImpl*, void*, size_t)> copy_data_function_;

  Comm() = default;
  Comm* do_start() override;

  static xbt::signal<void(Comm const&)> on_send;
  xbt::signal<void(Comm const&)> on_this_send;
  static xbt::signal<void(Comm const&)> on_recv;
  xbt::signal<void(Comm const&)> on_this_recv;
  inline static xbt::signal<void(Comm const&)> on_start;
  xbt::signal<void(Comm const&)> on_this_start;

protected:
  void fire_on_completion() const override {
    /* The completion signal of a Comm has to be thrown only once and not by the sender AND the receiver.
       then Comm::on_completion is thrown in the kernel in CommImpl::finish.
     */
  }
  void fire_on_this_completion() const override {
    /* The completion signal of a Comm has to be thrown only once and not by the sender AND the receiver.
       then Comm::on_completion is thrown in the kernel in CommImpl::finish.
     */
  }
  void fire_on_suspend() const override { on_suspend(*this); }
  void fire_on_this_suspend() const override { on_this_suspend(*this); }
  void fire_on_resume() const override { on_resume(*this); }
  void fire_on_this_resume() const override { on_this_resume(*this); }
  void fire_on_veto() const override { on_veto(const_cast<Comm&>(*this)); }
  void fire_on_this_veto() const override { on_this_veto(const_cast<Comm&>(*this)); }

public:
  /*! \static Add a callback fired when the send of any Comm is posted  */
  static void on_send_cb(const std::function<void(Comm const&)>& cb) { on_send.connect(cb); }
  /*! Add a callback fired when the send of this specific Comm is posted  */
  void on_this_send_cb(const std::function<void(Comm const&)>& cb) { on_this_send.connect(cb); }
  /*! \static Add a callback fired when the recv of any Comm is posted  */
  static void on_recv_cb(const std::function<void(Comm const&)>& cb) { on_recv.connect(cb); }
  /*! Add a callback fired when the recv of this specific Comm is posted  */
  void on_this_recv_cb(const std::function<void(Comm const&)>& cb) { on_this_recv.connect(cb); }
  /*! \static Add a callback fired when any Comm starts  */
  static void on_start_cb(const std::function<void(Comm const&)>& cb) { on_start.connect(cb); }
  /*!  Add a callback fired when this specific Comm starts  */
  void on_this_start_cb(const std::function<void(Comm const&)>& cb) { on_this_start.connect(cb); }

  CommPtr set_copy_data_callback(const std::function<void(kernel::activity::CommImpl*, void*, size_t)>& callback);
  XBT_ATTRIB_DEPRECATED_v338("Please manifest if you actually need this function") static void copy_buffer_callback(
      kernel::activity::CommImpl*, void*, size_t);
  XBT_ATTRIB_DEPRECATED_v338("Please manifest if you actually need this function") static void copy_pointer_callback(
      kernel::activity::CommImpl*, void*, size_t);

  ~Comm() override;

  static void send(kernel::actor::ActorImpl* sender, const Mailbox* mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size,
                   const std::function<bool(void*, void*, simgrid::kernel::activity::CommImpl*)>& match_fun,
                   const std::function<void(simgrid::kernel::activity::CommImpl*, void*, size_t)>& copy_data_fun,
                   void* data, double timeout);
  static void recv(kernel::actor::ActorImpl* receiver, const Mailbox* mbox, void* dst_buff, size_t* dst_buff_size,
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
  static CommPtr sendto_init(Host* from, Host* to);
  static CommPtr sendto_async(Host* from, Host* to, uint64_t simulated_size_in_bytes);
  static void sendto(Host* from, Host* to, uint64_t simulated_size_in_bytes);

  CommPtr set_source(Host* from);
  Host* get_source() const;
  CommPtr set_destination(Host* to);
  Host* get_destination() const;

  /* Mailbox-based communications */
  CommPtr set_mailbox(Mailbox* mailbox);
  /** Retrieve the mailbox on which this comm acts */
  Mailbox* get_mailbox() const { return mailbox_; }

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
  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach();
  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach(const std::function<void(void*)>& clean_function)
  {
    clean_fun_ = clean_function;
    return detach();
  }

  Comm* wait_for(double timeout) override;

  /*! \static take a vector s4u::CommPtr and return the rank of the first finished one (or -1 if none is done). */
  static ssize_t test_any(const std::vector<CommPtr>& comms);

  /*! \static take a vector s4u::CommPtr and return when one of them is finished.
   * The return value is the rank of the first finished CommPtr. */
  static ssize_t wait_any(const std::vector<CommPtr>& comms) { return wait_any_for(comms, -1); }
  /*! \static Same as wait_any, but with a timeout. Return -1 if the timeout occurs.*/
  static ssize_t wait_any_for(const std::vector<CommPtr>& comms, double timeout);

  /*! \static take a vector s4u::CommPtr and return when all of them is finished. */
  static void wait_all(const std::vector<CommPtr>& comms);
  /*! \static Same as wait_all, but with a timeout. Return the number of terminated comm (less than comms.size() if
   *  the timeout occurs). */
  static size_t wait_all_for(const std::vector<CommPtr>& comms, double timeout);
};
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COMM_HPP */
