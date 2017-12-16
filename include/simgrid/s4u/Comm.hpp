/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <xbt/base.h>

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Mailbox.hpp> // DEPRECATED 3.20
#include <simgrid/s4u/forward.hpp>

#include <vector>

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
  friend XBT_PUBLIC(void) intrusive_ptr_release(simgrid::s4u::Comm* c);
  friend XBT_PUBLIC(void) intrusive_ptr_add_ref(simgrid::s4u::Comm* c);
  friend Mailbox; // Factory of comms

  virtual ~Comm();

  /*! take a vector s4u::CommPtr and return when one of them is finished.
   * The return value is the rank of the first finished CommPtr. */
  static int wait_any(std::vector<CommPtr> * comms) { return wait_any_for(comms, -1); }
  /*! Same as wait_any, but with a timeout. If the timeout occurs, parameter last is returned.*/
  static int wait_any_for(std::vector<CommPtr> * comms_in, double timeout)
  {
    // Map to dynar<Synchro*>:
    xbt_dynar_t comms = xbt_dynar_new(sizeof(simgrid::kernel::activity::ActivityImpl*), [](void*ptr){
      intrusive_ptr_release(*(simgrid::kernel::activity::ActivityImpl**)ptr);
    });
    for (auto const& comm : *comms_in) {
      if (comm->state_ == inited)
        comm->start();
      xbt_assert(comm->state_ == started);
      simgrid::kernel::activity::ActivityImpl* ptr = comm->pimpl_.get();
      intrusive_ptr_add_ref(ptr);
      xbt_dynar_push_as(comms, simgrid::kernel::activity::ActivityImpl*, ptr);
    }
    // Call the underlying simcall:
    int idx = simcall_comm_waitany(comms, timeout);
    xbt_dynar_free(&comms);
    return idx;
  }

  /*! take a vector s4u::CommPtr and return when all of them is finished. */
  static void wait_all(std::vector<CommPtr> * comms)
  {
    // TODO: this should be a simcall or something
    // TODO: we are missing a version with timeout
    for (CommPtr comm : *comms) {
      comm->wait();
    }
  }

  /** Creates (but don't start) an async send to the mailbox @p dest */
  XBT_ATTRIB_DEPRECATED_v320("Use Mailbox::put_init(): v3.20 will turn this warning into an error.") static CommPtr
  send_init(MailboxPtr dest)
  {
    return dest->put_init();
  }
  /** Creates (but don't start) an async send to the mailbox @p dest */
  XBT_ATTRIB_DEPRECATED_v320("Use Mailbox::put_init(): v3.20 will turn this warning into an error.") static CommPtr
  send_init(MailboxPtr dest, void* data, int simulatedByteAmount)
  {
    return dest->put_init(data, simulatedByteAmount);
  }
  /** Creates and start an async send to the mailbox @p dest */
  XBT_ATTRIB_DEPRECATED_v320("Use Mailbox::put_async(): v3.20 will turn this warning into an error.") static CommPtr
  send_async(MailboxPtr dest, void* data, int simulatedByteAmount)
  {
    return dest->put_async(data, simulatedByteAmount);
  }
  /** Creates (but don't start) an async recv onto the mailbox @p from */
  XBT_ATTRIB_DEPRECATED_v320("Use Mailbox::get_init(): v3.20 will turn this warning into an error.") static CommPtr
  recv_init(MailboxPtr from)
  {
    return from->get_init();
  }
  /** Creates and start an async recv to the mailbox @p from */
  XBT_ATTRIB_DEPRECATED_v320("Use Mailbox::get_async(): v3.20 will turn this warning into an error.") static CommPtr
  recv_async(MailboxPtr from, void** data)
  {
    return from->get_async(data);
  }

  Activity* start() override;
  Activity* wait() override;
  Activity* wait(double timeout) override;

  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Activity* detach();
  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  Activity* detach(void (*cleanFunction)(void*))
  {
    cleanFunction_ = cleanFunction;
    return detach();
  }

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  Activity* setRate(double rate);

  /** Specify the data to send */
  Activity* setSrcData(void* buff);
  /** Specify the size of the data to send */
  Activity* setSrcDataSize(size_t size);
  /** Specify the data to send and its size */
  Activity* setSrcData(void* buff, size_t size);

  /** Specify where to receive the data */
  Activity* setDstData(void** buff);
  /** Specify the buffer in which the data should be received */
  Activity* setDstData(void** buff, size_t size);
  /** Retrieve the size of the received data */
  size_t getDstDataSize();

  bool test();
  Activity* cancel();

  /** Retrieve the mailbox on which this comm acts */
  MailboxPtr getMailbox();

private:
  double rate_        = -1;
  void* dstBuff_      = nullptr;
  size_t dstBuffSize_ = 0;
  void* srcBuff_      = nullptr;
  size_t srcBuffSize_ = sizeof(void*);

  /* FIXME: expose these elements in the API */
  int detached_ = 0;
  int (*matchFunction_)(void*, void*, simgrid::kernel::activity::CommImpl*) = nullptr;
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
