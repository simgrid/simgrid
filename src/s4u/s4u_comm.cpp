/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/comm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm,s4u_async,"S4U asynchronous communications");
using namespace simgrid;

s4u::Comm::~Comm() {

}

s4u::Comm &s4u::Comm::send_init(s4u::Actor *sender, s4u::Mailbox &chan) {
  s4u::Comm *res = new s4u::Comm();
  res->sender_ = sender;
  res->mailbox_ = &chan;

  return *res;
}
s4u::Comm &s4u::Comm::recv_init(s4u::Actor *receiver, s4u::Mailbox &chan) {
  s4u::Comm *res = new s4u::Comm();
  res->receiver_ = receiver;
  res->mailbox_ = &chan;

  return *res;
}

void s4u::Comm::setRate(double rate) {
  xbt_assert(p_state==inited);
  rate_ = rate;
}

void s4u::Comm::setSrcData(void * buff) {
  xbt_assert(p_state==inited);
  xbt_assert(dstBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
}
void s4u::Comm::setSrcDataSize(size_t size){
  xbt_assert(p_state==inited);
  srcBuffSize_ = size;
}
void s4u::Comm::setSrcData(void * buff, size_t size) {
  xbt_assert(p_state==inited);

  xbt_assert(dstBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
  srcBuffSize_ = size;
}
void s4u::Comm::setDstData(void ** buff) {
  xbt_assert(p_state==inited);
  xbt_assert(srcBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
}
size_t s4u::Comm::getDstDataSize(){
  xbt_assert(p_state==finished);
  return dstBuffSize_;
}
void s4u::Comm::setDstData(void ** buff, size_t size) {
  xbt_assert(p_state==inited);

  xbt_assert(srcBuff_ == NULL, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
  dstBuffSize_ = size;
}

void s4u::Comm::start() {
  xbt_assert(p_state == inited);

  if (srcBuff_ != NULL) { // Sender side
    p_inferior = simcall_comm_isend(sender_->getInferior(), mailbox_->getInferior(), p_remains, rate_,
        srcBuff_, srcBuffSize_,
        matchFunction_, cleanFunction_, copyDataFunction_,
        p_userData, detached_);
  } else if (dstBuff_ != NULL) { // Receiver side
    p_inferior = simcall_comm_irecv(receiver_->getInferior(), mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
        matchFunction_, copyDataFunction_,
        p_userData, rate_);

  } else {
    xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
  }
  p_state = started;
}
void s4u::Comm::wait() {
  xbt_assert(p_state == started || p_state == inited);

  if (p_state == started)
    simcall_comm_wait(p_inferior, -1/*timeout*/);
  else {// p_state == inited. Save a simcall and do directly a blocking send/recv
    if (srcBuff_ != NULL) {
      simcall_comm_send(sender_->getInferior(), mailbox_->getInferior(), p_remains, rate_,
          srcBuff_, srcBuffSize_,
          matchFunction_, copyDataFunction_,
          p_userData, -1 /*timeout*/);
    } else {
      simcall_comm_recv(receiver_->getInferior(), mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
          matchFunction_, copyDataFunction_,
          p_userData, -1/*timeout*/, rate_);
    }
  }
  p_state = finished;
}
void s4u::Comm::wait(double timeout) {
  xbt_assert(p_state == started || p_state == inited);

  if (p_state == started) {
    simcall_comm_wait(p_inferior, timeout);
    p_state = finished;
    return;
  }

  // It's not started yet. Do it in one simcall
  if (srcBuff_ != NULL) {
    simcall_comm_send(sender_->getInferior(), mailbox_->getInferior(), p_remains, rate_,
        srcBuff_, srcBuffSize_,
        matchFunction_, copyDataFunction_,
        p_userData, timeout);
  } else { // Receiver
    simcall_comm_recv(receiver_->getInferior(), mailbox_->getInferior(), dstBuff_, &dstBuffSize_,
        matchFunction_, copyDataFunction_,
        p_userData, timeout, rate_);
  }
  p_state = finished;
}

s4u::Comm &s4u::Comm::send_async(s4u::Actor *sender, Mailbox &dest, void *data, int simulatedSize) {
  s4u::Comm &res = s4u::Comm::send_init(sender, dest);

  res.setRemains(simulatedSize);
  res.srcBuff_ = data;
  res.srcBuffSize_ = sizeof(void*);

  res.start();
  return res;
}

s4u::Comm &s4u::Comm::recv_async(s4u::Actor *receiver, Mailbox &dest, void **data) {
  s4u::Comm &res = s4u::Comm::recv_init(receiver, dest);

  res.setDstData(data);

  res.start();
  return res;
}

