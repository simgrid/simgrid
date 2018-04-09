/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm,s4u_activity,"S4U asynchronous communications");

namespace simgrid {
namespace s4u {
Comm::~Comm()
{
  if (state_ == State::started && not detached_ && (pimpl_ == nullptr || pimpl_->state == SIMIX_RUNNING)) {
    XBT_INFO("Comm %p freed before its completion. Detached: %d, State: %d", this, detached_, (int)state_);
    if (pimpl_ != nullptr)
      XBT_INFO("pimpl_->state: %d", pimpl_->state);
    else
      XBT_INFO("pimpl_ is null");
    xbt_backtrace_display_current();
  }
}

Activity* Comm::setRate(double rate)
{
  xbt_assert(state_ == State::inited);
  rate_ = rate;
  return this;
}

Activity* Comm::setSrcData(void* buff)
{
  xbt_assert(state_ == State::inited);
  xbt_assert(dstBuff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
  return this;
}
Activity* Comm::setSrcDataSize(size_t size)
{
  xbt_assert(state_ == State::inited);
  srcBuffSize_ = size;
  return this;
}
Activity* Comm::setSrcData(void* buff, size_t size)
{
  xbt_assert(state_ == State::inited);

  xbt_assert(dstBuff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  srcBuff_ = buff;
  srcBuffSize_ = size;
  return this;
}
Activity* Comm::setDstData(void** buff)
{
  xbt_assert(state_ == State::inited);
  xbt_assert(srcBuff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
  return this;
}
size_t Comm::getDstDataSize(){
  xbt_assert(state_ == State::finished);
  return dstBuffSize_;
}
Activity* Comm::setDstData(void** buff, size_t size)
{
  xbt_assert(state_ == State::inited);

  xbt_assert(srcBuff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  dstBuff_ = buff;
  dstBuffSize_ = size;
  return this;
}

Activity* Comm::start()
{
  xbt_assert(state_ == State::inited);

  if (srcBuff_ != nullptr) { // Sender side
    pimpl_ = simcall_comm_isend(sender_, mailbox_->get_impl(), remains_, rate_, srcBuff_, srcBuffSize_, matchFunction_,
                                cleanFunction_, copyDataFunction_, user_data_, detached_);
  } else if (dstBuff_ != nullptr) { // Receiver side
    xbt_assert(not detached_, "Receive cannot be detached");
    pimpl_ = simcall_comm_irecv(receiver_, mailbox_->get_impl(), dstBuff_, &dstBuffSize_, matchFunction_,
                                copyDataFunction_, user_data_, rate_);

  } else {
    xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
  }
  state_ = State::started;
  return this;
}

/** @brief Block the calling actor until the communication is finished */
Activity* Comm::wait()
{
  return this->wait(-1);
}

/** @brief Block the calling actor until the communication is finished, or until timeout
 *
 * On timeout, an exception is thrown.
 *
 * @param timeout the amount of seconds to wait for the comm termination.
 *                Negative values denote infinite wait times. 0 as a timeout returns immediately. */
Activity* Comm::wait(double timeout)
{
  switch (state_) {
    case State::finished:
      return this;

    case State::inited: // It's not started yet. Do it in one simcall
      if (srcBuff_ != nullptr) {
        simcall_comm_send(sender_, mailbox_->get_impl(), remains_, rate_, srcBuff_, srcBuffSize_, matchFunction_,
                          copyDataFunction_, user_data_, timeout);
      } else { // Receiver
        simcall_comm_recv(receiver_, mailbox_->get_impl(), dstBuff_, &dstBuffSize_, matchFunction_, copyDataFunction_,
                          user_data_, timeout, rate_);
      }
      state_ = State::finished;
      return this;

    case State::started:
      simcall_comm_wait(pimpl_, timeout);
      state_ = State::finished;
      return this;

    default:
      THROW_IMPOSSIBLE;
  }
  return this;
}
int Comm::test_any(std::vector<CommPtr>* comms)
{
  smx_activity_t* array = new smx_activity_t[comms->size()];
  for (unsigned int i = 0; i < comms->size(); i++) {
    array[i] = comms->at(i)->pimpl_;
  }
  int res = simcall_comm_testany(array, comms->size());
  delete[] array;
  return res;
}

Activity* Comm::detach()
{
  xbt_assert(state_ == State::inited, "You cannot detach communications once they are started (not implemented).");
  xbt_assert(srcBuff_ != nullptr && srcBuffSize_ != 0, "You can only detach sends, not recvs");
  detached_ = true;
  return start();
}

Activity* Comm::cancel()
{
  simgrid::kernel::activity::CommImplPtr commPimpl =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(pimpl_);
  commPimpl->cancel();
  return this;
}

bool Comm::test()
{
  xbt_assert(state_ == State::inited || state_ == State::started || state_ == State::finished);

  if (state_ == State::finished)
    return true;

  if (state_ == State::inited)
    this->start();

  if(simcall_comm_test(pimpl_)){
    state_ = State::finished;
    return true;
  }
  return false;
}

MailboxPtr Comm::getMailbox()
{
  return mailbox_;
}

void intrusive_ptr_release(simgrid::s4u::Comm* c)
{
  if (c->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete c;
  }
}
void intrusive_ptr_add_ref(simgrid::s4u::Comm* c)
{
  c->refcount_.fetch_add(1, std::memory_order_relaxed);
}
}
} // namespaces
