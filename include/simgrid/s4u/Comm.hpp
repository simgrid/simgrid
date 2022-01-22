/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>
#include <vector>

namespace simgrid {
namespace s4u {
/** @brief Communication async
 *
 * Represents all asynchronous communications, that you can test or wait onto.
 */
class XBT_PUBLIC Comm : public Activity_T<Comm> {
  Mailbox* mailbox_                   = nullptr;
  kernel::actor::ActorImpl* sender_   = nullptr; /* specified for normal mailbox-based communications*/
  kernel::actor::ActorImpl* receiver_ = nullptr;
  Host* from_                         = nullptr; /* specified only for direct host-to-host communications */
  Host* to_                           = nullptr;
  double rate_                        = -1;
  void* dst_buff_                     = nullptr;
  size_t dst_buff_size_               = 0;
  void* src_buff_                     = nullptr;
  size_t src_buff_size_               = sizeof(void*);
  /* FIXME: expose these elements in the API */
  bool detached_                                                          = false;
  bool (*match_fun_)(void*, void*, kernel::activity::CommImpl*)           = nullptr;
  void (*clean_fun_)(void*)                                               = nullptr;
  void (*copy_data_function_)(kernel::activity::CommImpl*, void*, size_t) = nullptr;

  Comm() = default;

public:
#ifndef DOXYGEN
  friend Mailbox; // Factory of comms
#endif

  ~Comm() override;

  /*! Creates a communication that bypasses the mailbox mechanism. */
  static CommPtr sendto_init();
  /*! Creates a communication beween the two given hosts, bypassing the mailbox mechanism. */
  static CommPtr sendto_init(Host* from, Host* to);
  /** Do an asynchronous communication between two arbitrary hosts.
   *
   * This initializes a communication that completely bypass the mailbox and actors mechanism.
   * There is really no limit on the hosts involved. In particular, the actor does not have to be on one of the involved
   * hosts.
   */
  static CommPtr sendto_async(Host* from, Host* to, uint64_t simulated_size_in_bytes);
  /** Do a blocking communication between two arbitrary hosts.
   *
   * This starts a blocking communication right away, bypassing the mailbox and actors mechanism.
   * The calling actor is blocked until the end of the communication; there is really no limit on the hosts involved.
   * In particular, the actor does not have to be on one of the involved hosts. Enjoy the comfort of the simulator :)
   */
  static void sendto(Host* from, Host* to, uint64_t simulated_size_in_bytes);

  static void on_send_cb(const std::function<void(Comm const&)>& cb) { on_send.connect(cb); }
  static void on_recv_cb(const std::function<void(Comm const&)>& cb) { on_recv.connect(cb); }
  static void on_start_cb(const std::function<void(Comm const&)>& cb) { on_start.connect(cb); }
  static void on_completion_cb(const std::function<void(Activity const&)>& cb) { on_completion.connect(cb); }
#ifndef DOXYGEN
  /* FIXME signals should be private */
  static xbt::signal<void(Comm const&)> on_send;
  static xbt::signal<void(Comm const&)> on_recv;
  static xbt::signal<void(Comm const&)> on_start;
  static xbt::signal<void(Comm const&)> on_completion;
#endif

  /*! take a vector s4u::CommPtr and return when one of them is finished.
   * The return value is the rank of the first finished CommPtr. */
  static ssize_t wait_any(const std::vector<CommPtr>& comms) { return wait_any_for(comms, -1); }
  /*! Same as wait_any, but with a timeout. Return -1 if the timeout occurs.*/
  static ssize_t wait_any_for(const std::vector<CommPtr>& comms, double timeout);

  /*! take a vector s4u::CommPtr and return when all of them is finished. */
  static void wait_all(const std::vector<CommPtr>& comms);
  /*! Same as wait_all, but with a timeout. Return the number of terminated comm (less than comms.size() if the timeout
   * occurs). */
  static size_t wait_all_for(const std::vector<CommPtr>& comms, double timeout);
  /*! take a vector s4u::CommPtr and return the rank of the first finished one (or -1 if none is done). */
  static ssize_t test_any(const std::vector<CommPtr>& comms);

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v332("Please use a plain vector for parameter")
  static int wait_any(const std::vector<CommPtr>* comms) { return static_cast<int>(wait_any_for(*comms, -1)); }
  XBT_ATTRIB_DEPRECATED_v332("Please use a plain vector for first parameter")
  static int wait_any_for(const std::vector<CommPtr>* comms, double timeout) { return static_cast<int>(wait_any_for(*comms, timeout)); }
  XBT_ATTRIB_DEPRECATED_v332("Please use a plain vector for parameter")
  static void wait_all(const std::vector<CommPtr>* comms) { wait_all(*comms); }
  XBT_ATTRIB_DEPRECATED_v332("Please use a plain vector for parameter")
  static int test_any(const std::vector<CommPtr>* comms) { return static_cast<int>(test_any(*comms)); }
#endif

  Comm* start() override;
  Comm* wait_for(double timeout) override;
  bool test() override;

  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach();
  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach(void (*clean_function)(void*))
  {
    clean_fun_ = clean_function;
    return detach();
  }

  /** Set the source and destination of communications that bypass the mailbox mechanism */
  CommPtr set_source(Host* from);
  Host* get_source() const { return from_; }
  CommPtr set_destination(Host* to);
  Host* get_destination() const { return to_; }

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  CommPtr set_rate(double rate);

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

  /** Specify the amount of bytes which exchange should be simulated (not to be mixed with set_src_data_size())
   *
   * @beginrst
   * That's the size of the simulated data, that's completely related from the actual data size (given by
   * :cpp:func:`simgrid::s4u::Comm::set_src_data_size`).
   * @endrst
   */
  CommPtr set_payload_size(uint64_t bytes);

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
  void* get_dst_data();

  /** Retrieve the mailbox on which this comm acts */
  Mailbox* get_mailbox() const;
  /** Retrieve the size of the received data. Not to be mixed with @ref Activity::set_remaining()  */
  size_t get_dst_data_size() const;

  Actor* get_sender() const;

  bool is_assigned() const override { return (to_ != nullptr && from_ != nullptr) || (mailbox_ != nullptr); }

  CommPtr set_copy_data_callback(void (*callback)(kernel::activity::CommImpl*, void*, size_t));
  static void copy_buffer_callback(kernel::activity::CommImpl*, void*, size_t);
  static void copy_pointer_callback(kernel::activity::CommImpl*, void*, size_t);
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_COMM_HPP */
