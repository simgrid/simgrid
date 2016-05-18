/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/comm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm,s4u_activity,"S4U asynchronous communications");
using namespace simgrid;

s4u::Comm::~Comm() {

}

s4u::Comm &s4u::Comm::send_init(s4u::Mailbox &chan) {
  s4u::Comm *res = new s4u::Comm();
  res->sender_ = SIMIX_process_self();
  res->mailbox_ = &chan;
  return *res;
}

s4u::Comm &s4u::Comm::recv_init(s4u::Mailbox &chan) {
  s4u::Comm *res = new s4u::Comm();
  res->receiver_ = SIMIX_process_self();
  res->mailbox_ = &chan;
  return *res;
}

void s4u::Comm::setRate(double rate) {
  xbt_assert(state_==inited);
  rate_ = rate;
}

void s4u::Comm::setSrcData(void * buff) {
  xbt_assert(state_==inited);
  xbt_assert(dstBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
}
void s4u::Comm::setSrcDataSize(size_t size){
  xbt_assert(state_==inited);
  srcBuffSize_ = size;
}
void s4u::Comm::setSrcData(void * buff, size_t size) {
  xbt_assert(state_==inited);

  xbt_assert(dstBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
  srcBuffSize_ = size;
}
void s4u::Comm::setDstData(void ** buff) {
  xbt_assert(state_==inited);
  xbt_assert(srcBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
}
size_t s4u::Comm::getDstDataSize(){
  xbt_assert(state_==finished);
  return dstBuffSize_;
}
void s4u::Comm::setDstData(void ** buff, size_t size) {
  xbt_assert(state_==inited);

  xbt_assert(srcBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
  dstBuffSize_ = size;
}

void s4u::Comm::start() {
  xbt_assert(state_ == inited);

  if (srcBuff_ != NULL) { // Sender side
    pimpl_ = simcall_comm_isend(sender_, mailbox_->getInferior(), remains_, rate_,
        srcBuff_, srcBuffSize_,
        matchFunction_, cleanFunction_, copyDataFunction_,
        userData_, detached_);
  } else if (dstBuff_ != NULL) { // Receiver side
    pimpl_ = simcall_comm_irecv(receiver_, mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
        matchFunction_, copyDataFunction_,
        userData_, rate_);

  } else {
    xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
  }
  state_ = started;
}
void s4u::Comm::wait() {
  xbt_assert(state_ == started || state_ == inited);

  if (state_ == started)
    simcall_comm_wait(pimpl_, -1/*timeout*/);
  else {// p_state == inited. Save a simcall and do directly a blocking send/recv
    if (srcBuff_ != NULL) {
      simcall_comm_send(sender_, mailbox_->getInferior(), remains_, rate_,
          srcBuff_, srcBuffSize_,
          matchFunction_, copyDataFunction_,
          userData_, -1 /*timeout*/);
    } else {
      simcall_comm_recv(receiver_, mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
          matchFunction_, copyDataFunction_,
          userData_, -1/*timeout*/, rate_);
    }
  }
  state_ = finished;
}
void s4u::Comm::wait(double timeout) {
  xbt_assert(state_ == started || state_ == inited);

  if (state_ == started) {
    simcall_comm_wait(pimpl_, timeout);
    state_ = finished;
    return;
  }

  // It's not started yet. Do it in one simcall
  if (srcBuff_ != NULL) {
    simcall_comm_send(sender_, mailbox_->getInferior(), remains_, rate_,
        srcBuff_, srcBuffSize_,
        matchFunction_, copyDataFunction_,
        userData_, timeout);
  } else { // Receiver
    simcall_comm_recv(receiver_, mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
        matchFunction_, copyDataFunction_,
        userData_, timeout, rate_);
  }
  state_ = finished;
}

s4u::Comm &s4u::Comm::send_async(Mailbox &dest, void *data, int simulatedSize) {
  s4u::Comm &res = s4u::Comm::send_init(dest);
  res.setRemains(simulatedSize);
  res.srcBuff_ = data;
  res.srcBuffSize_ = sizeof(void*);
  res.start();
  return res;
}

s4u::Comm &s4u::Comm::recv_async(Mailbox &dest, void **data) {
  s4u::Comm &res = s4u::Comm::recv_init(dest);
  res.setDstData(data);
  res.start();
  return res;
}

