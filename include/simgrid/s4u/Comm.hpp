/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

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

  /*! Creates a communication beween the two given hosts, bypassing the mailbox mechanism. */
  static CommPtr sendto_init(Host* from, Host* to);
  /*! Creates and start a communication of the given amount of bytes beween the two given hosts, bypassing the mailbox
   * mechanism */
  static CommPtr sendto_async(Host* from, Host* to, double simulated_size_in_bytes);

  static xbt::signal<void(Comm const&, bool is_sender)> on_start;
  static xbt::signal<void(Comm const&)> on_completion;

  /*! take a vector s4u::CommPtr and return when one of them is finished.
   * The return value is the rank of the first finished CommPtr. */
  static int wait_any(const std::vector<CommPtr>* comms) { return wait_any_for(comms, -1); }
  /*! Same as wait_any, but with a timeout. Return -1 if the timeout occurs.*/
  static int wait_any_for(const std::vector<CommPtr>* comms_in, double timeout);

  /*! take a vector s4u::CommPtr and return when all of them is finished. */
  static void wait_all(const std::vector<CommPtr>* comms);
  /*! take a vector s4u::CommPtr and return the rank of the first finished one (or -1 if none is done). */
  static int test_any(const std::vector<CommPtr>* comms);

  Comm* start() override;
  Comm* wait() override;
  Comm* wait_for(double timeout) override;
  Comm* cancel() override;
  bool test() override;

  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach();
  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Comm* detach(void (*clean_function)(void*))
  {
    clean_fun_ = clean_function;
    return detach();
  }

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  CommPtr set_rate(double rate);

  /** Specify the data to send.
   *
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   */
  CommPtr set_src_data(void* buff);
  /** Specify the size of the data to send. Not to be mixed with @ref Activity::set_remaining()
   *
   * That's the size of the data to actually copy in the simulator (ie, the data passed with Activity::set_src_data()).
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   */
  CommPtr set_src_data_size(size_t size);
  /** Specify the data to send and its size. Don't mix the size with @ref Activity::set_remaining()
   *
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
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
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_COMM_HPP */
