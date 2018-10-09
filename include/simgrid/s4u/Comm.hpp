/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <atomic>
#include <vector>

namespace simgrid {
namespace s4u {
/** @brief Communication async
 *
 * Represents all asynchronous communications, that you can test or wait onto.
 */
class XBT_PUBLIC Comm : public Activity {
  Comm() : Activity() {}
public:
  friend XBT_PUBLIC void intrusive_ptr_release(simgrid::s4u::Comm * c);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(simgrid::s4u::Comm * c);
  friend simgrid::s4u::Mailbox; // Factory of comms

  virtual ~Comm();

  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_sender_start;
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_receiver_start;
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_completion;

  /*! take a vector s4u::CommPtr and return when one of them is finished.
   * The return value is the rank of the first finished CommPtr. */
  static int wait_any(std::vector<CommPtr> * comms) { return wait_any_for(comms, -1); }
  /*! Same as wait_any, but with a timeout. If the timeout occurs, parameter last is returned.*/
  static int wait_any_for(std::vector<CommPtr>* comms_in, double timeout);

  /*! take a vector s4u::CommPtr and return when all of them is finished. */
  static void wait_all(std::vector<CommPtr>* comms);
  /*! take a vector s4u::CommPtr and return the rank of the first finished one (or -1 if none is done). */
  static int test_any(std::vector<CommPtr> * comms);

  Comm* start() override;
  Comm* wait() override;
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

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  Comm* set_rate(double rate);

  /** Specify the data to send.
   *
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   */
  Comm* set_src_data(void* buff);
  /** Specify the size of the data to send. Not to be mixed with @ref Activity::set_remaining()
   *
   * That's the size of the data to actually copy in the simulator (ie, the data passed with Activity::set_src_data()).
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   */
  Comm* set_src_data_size(size_t size);
  /** Specify the data to send and its size. Don't mix the size with @ref Activity::set_remaining()
   *
   * This is way will get actually copied over to the receiver.
   * That's completely unrelated from the simulated size (given with @ref Activity::set_remaining()):
   * you can send a short buffer in your simulator, that represents a very large message
   * in the simulated world, or the opposite.
   */
  Comm* set_src_data(void* buff, size_t size);

  /** Specify where to receive the data.
   *
   * That's a buffer where the sent data will be copied */
  Comm* set_dst_data(void** buff);
  /** Specify the buffer in which the data should be received
   *
   * That's a buffer where the sent data will be copied  */
  Comm* set_dst_data(void** buff, size_t size);
  /** Retrieve the size of the received data. Not to be mixed with @ref Activity::set_remaining()  */
  size_t get_dst_data_size();

  Comm* cancel() override;

  /** Retrieve the mailbox on which this comm acts */
  MailboxPtr get_mailbox();

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v324("Please use Comm::wait_for()") void wait(double t) override { wait_for(t); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_rate()") Activity* setRate(double rate) { return set_rate(rate); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_src_data()") Activity* setSrcData(void* buff)
  {
    return set_src_data(buff);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_src_data()") Activity* setSrcData(void* buff, size_t size)
  {
    return set_src_data(buff, size);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_src_data_size()") Activity* setSrcDataSize(size_t size)
  {
    return set_src_data_size(size);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_dst_data()") Activity* setDstData(void** buff)
  {
    return set_dst_data(buff);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::set_dst_data()") Activity* setDstData(void** buff, size_t size)
  {
    return set_dst_data(buff, size);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::get_dst_data_size()") size_t getDstDataSize()
  {
    return get_dst_data_size();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Comm::get_mailbox()") MailboxPtr getMailbox() { return get_mailbox(); }
#endif

private:
  double rate_        = -1;
  void* dst_buff_       = nullptr;
  size_t dst_buff_size_ = 0;
  void* src_buff_       = nullptr;
  size_t src_buff_size_ = sizeof(void*);

  /* FIXME: expose these elements in the API */
  int detached_ = 0;
  int (*match_fun_)(void*, void*, simgrid::kernel::activity::CommImpl*) = nullptr;
  void (*clean_fun_)(void*)                                             = nullptr;
  void (*copy_data_function_)(smx_activity_t, void*, size_t)            = nullptr;

  smx_actor_t sender_   = nullptr;
  smx_actor_t receiver_ = nullptr;
  MailboxPtr mailbox_   = nullptr;

  std::atomic_int_fast32_t refcount_{0};
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_COMM_HPP */
