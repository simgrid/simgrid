/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COMM_HPP
#define SIMGRID_S4U_COMM_HPP

#include <xbt/base.h>
#include <simgrid/s4u/forward.hpp>
#include <simgrid/s4u/async.hpp>
#include <simgrid/s4u/mailbox.hpp>

namespace simgrid {
namespace s4u {

/** @brief Communication async
 *
 * Represents all asynchronous communications, that you can test or wait onto.
 */
XBT_PUBLIC_CLASS Comm : public Async {
  Comm() : Async() {}
public:
  virtual ~Comm();

public:
  /** Creates (but don't start) an async send to the mailbox #dest */
  static Comm &send_init(Actor *sender, Mailbox &dest);
  /** Creates and start an async send to the mailbox #dest */
  static Comm &send_async(s4u::Actor *sender, Mailbox &dest, void *data, int simulatedByteAmount);
    /** Creates (but don't start) an async recv onto the mailbox #from */
  static Comm &recv_init(s4u::Actor *receiver, Mailbox &from);
  /** Creates and start an async recv to the mailbox #from */
  static Comm &recv_async(s4u::Actor *receiver, Mailbox &from, void **data);

  void start() override;
  void wait() override;
  void wait(double timeout) override;

private:
  double rate_=-1;
public:
  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  void setRate(double rate);

private:
  void *dstBuff_ = NULL;
  size_t dstBuffSize_ = 0;
  void *srcBuff_ = NULL;
  size_t srcBuffSize_ = sizeof(void*);
public:
  /** Specify the data to send */
  void setSrcData(void * buff);
  /** Specify the size of the data to send */
  void setSrcDataSize(size_t size);
  /** Specify the data to send and its size */
  void setSrcData(void * buff, size_t size);

  /** Specify where to receive the data */
  void setDstData(void ** buff);
  /** Specify the buffer in which the data should be received */
  void setDstData(void ** buff, size_t size);
  /** Retrieve the size of the received data */
  size_t getDstDataSize();


private: /* FIXME: expose these elements in the API */
  int detached_ = 0;
  int (*matchFunction_)(void *, void *, smx_synchro_t) = NULL;
  void (*cleanFunction_)(void *) = NULL;
  void (*copyDataFunction_)(smx_synchro_t, void*, size_t) = NULL;

private:
  Actor *sender_ = NULL;
  Actor *receiver_ = NULL;
  Mailbox *mailbox_ = NULL;
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COMM_HPP */
