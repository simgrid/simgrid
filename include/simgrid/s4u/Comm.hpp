/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <xbt/base.h>

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/forward.hpp>
namespace simgrid {
namespace s4u {
/** @brief Communication async
 *
 * Represents all asynchronous communications, that you can test or wait onto.
 */
XBT_PUBLIC_CLASS Comm : public Activity
{
  Comm() : Activity() {}
public:
  friend void intrusive_ptr_release(simgrid::s4u::Comm * c);
  friend void intrusive_ptr_add_ref(simgrid::s4u::Comm * c);

  virtual ~Comm();

  /*! take a range of s4u::Comm* (last excluded) and return when one of them is finished. The return value is an
   * iterator on the finished Comms. */
  template <class I> static I wait_any(I first, I last)
  {
    // Map to dynar<Synchro*>:
    xbt_dynar_t comms = xbt_dynar_new(sizeof(simgrid::kernel::activity::ActivityImpl*), NULL);
    for (I iter = first; iter != last; iter++) {
      Comm& comm = **iter;
      if (comm.state_ == inited)
        comm.start();
      xbt_assert(comm.state_ == started);
      xbt_dynar_push_as(comms, simgrid::kernel::activity::ActivityImpl*, comm.pimpl_);
    }
    // Call the underlying simcall:
    int idx = simcall_comm_waitany(comms, -1);
    xbt_dynar_free(&comms);
    // Not found:
    if (idx == -1)
      return last;
    // Lift the index to the corresponding iterator:
    auto res       = std::next(first, idx);
    (*res)->state_ = finished;
    return res;
  }
  /*! Same as wait_any, but with a timeout. If wait_any_for return because of the timeout last is returned.*/
  template <class I> static I wait_any_for(I first, I last, double timeout)
  {
    // Map to dynar<Synchro*>:
    xbt_dynar_t comms = xbt_dynar_new(sizeof(simgrid::kernel::activity::ActivityImpl*), NULL);
    for (I iter = first; iter != last; iter++) {
      Comm& comm = **iter;
      if (comm.state_ == inited)
        comm.start();
      xbt_assert(comm.state_ == started);
      xbt_dynar_push_as(comms, simgrid::kernel::activity::ActivityImpl*, comm.pimpl_);
    }
    // Call the underlying simcall:
    int idx = simcall_comm_waitany(comms, timeout);
    xbt_dynar_free(&comms);
    // Not found:
    if (idx == -1)
      return last;
    // Lift the index to the corresponding iterator:
    auto res       = std::next(first, idx);
    (*res)->state_ = finished;
    return res;
  }
  /** Creates (but don't start) an async send to the mailbox @p dest */
  static CommPtr send_init(MailboxPtr dest);
  /** Creates and start an async send to the mailbox @p dest */
  static CommPtr send_async(MailboxPtr dest, void* data, int simulatedByteAmount);
  /** Creates (but don't start) an async recv onto the mailbox @p from */
  static CommPtr recv_init(MailboxPtr from);
  /** Creates and start an async recv to the mailbox @p from */
  static CommPtr recv_async(MailboxPtr from, void** data);
  /** Creates and start a detached send to the mailbox @p dest
   *  TODO: make it possible to detach an already created comm */
  static void send_detached(MailboxPtr dest, void* data, int simulatedSize);

  void start() override;
  void wait() override;
  void wait(double timeout) override;

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  void setRate(double rate);

  /** Specify the data to send */
  void setSrcData(void* buff);
  /** Specify the size of the data to send */
  void setSrcDataSize(size_t size);
  /** Specify the data to send and its size */
  void setSrcData(void* buff, size_t size);

  /** Specify where to receive the data */
  void setDstData(void** buff);
  /** Specify the buffer in which the data should be received */
  void setDstData(void** buff, size_t size);
  /** Retrieve the size of the received data */
  size_t getDstDataSize();

  bool test();
  void cancel();

private:
  double rate_        = -1;
  void* dstBuff_      = nullptr;
  size_t dstBuffSize_ = 0;
  void* srcBuff_      = nullptr;
  size_t srcBuffSize_ = sizeof(void*);

  /* FIXME: expose these elements in the API */
  int detached_ = 0;
  int (*matchFunction_)(void*, void*, smx_activity_t) = nullptr;
  void (*cleanFunction_)(void*) = nullptr;
  void (*copyDataFunction_)(smx_activity_t, void*, size_t) = nullptr;

  smx_actor_t sender_   = nullptr;
  smx_actor_t receiver_ = nullptr;
  MailboxPtr mailbox_   = nullptr;

  std::atomic_int_fast32_t refcount_{0};
};
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COMM_HPP */
