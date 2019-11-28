/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm, s4u_activity, "S4U asynchronous communications");

namespace simgrid {
namespace s4u {
xbt::signal<void(Actor const&)> Comm::on_sender_start;
xbt::signal<void(Actor const&)> Comm::on_receiver_start;
xbt::signal<void(Actor const&)> Comm::on_completion;

Comm::~Comm()
{
  if (state_ == State::STARTED && not detached_ &&
      (pimpl_ == nullptr || pimpl_->state_ == kernel::activity::State::RUNNING)) {
    XBT_INFO("Comm %p freed before its completion. Detached: %d, State: %d", this, detached_, (int)state_);
    if (pimpl_ != nullptr)
      XBT_INFO("pimpl_->state: %d", static_cast<int>(pimpl_->state_));
    else
      XBT_INFO("pimpl_ is null");
    xbt_backtrace_display_current();
  }
}

int Comm::wait_any_for(std::vector<CommPtr>* comms, double timeout)
{
  std::unique_ptr<kernel::activity::CommImpl* []> rcomms(new kernel::activity::CommImpl*[comms->size()]);
  std::transform(begin(*comms), end(*comms), rcomms.get(),
                 [](const CommPtr& comm) { return static_cast<kernel::activity::CommImpl*>(comm->pimpl_.get()); });
  return simcall_comm_waitany(rcomms.get(), comms->size(), timeout);
}

void Comm::wait_all(std::vector<CommPtr>* comms)
{
  // TODO: this should be a simcall or something
  // TODO: we are missing a version with timeout
  for (CommPtr comm : *comms)
    comm->wait();
}

CommPtr Comm::set_rate(double rate)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  rate_ = rate;
  return this;
}

CommPtr Comm::set_src_data(void* buff)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  xbt_assert(dst_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  src_buff_ = buff;
  return this;
}

CommPtr Comm::set_src_data_size(size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  src_buff_size_ = size;
  return this;
}

CommPtr Comm::set_src_data(void* buff, size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  xbt_assert(dst_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  src_buff_      = buff;
  src_buff_size_ = size;
  return this;
}
CommPtr Comm::set_dst_data(void** buff)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  xbt_assert(src_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  dst_buff_ = buff;
  return this;
}

size_t Comm::get_dst_data_size()
{
  xbt_assert(state_ == State::FINISHED, "You cannot use %s before your communication terminated", __FUNCTION__);
  return dst_buff_size_;
}
CommPtr Comm::set_dst_data(void** buff, size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  xbt_assert(src_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  dst_buff_      = buff;
  dst_buff_size_ = size;
  return this;
}

CommPtr Comm::set_tracing_category(const std::string& category)
{
  xbt_assert(state_ == State::INITED, "Cannot change the tracing category of an exec after its start");
  tracing_category_ = category;
  return this;
}

Comm* Comm::start()
{
  xbt_assert(get_state() == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  if (src_buff_ != nullptr) { // Sender side
    on_sender_start(*Actor::self());
    pimpl_ = simcall_comm_isend(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                                clean_fun_, copy_data_function_, get_user_data(), detached_);
  } else if (dst_buff_ != nullptr) { // Receiver side
    xbt_assert(not detached_, "Receive cannot be detached");
    on_receiver_start(*Actor::self());
    pimpl_ = simcall_comm_irecv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_,
                                copy_data_function_, get_user_data(), rate_);

  } else {
    xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
  }
  state_ = State::STARTED;
  return this;
}

/** @brief Block the calling actor until the communication is finished */
Comm* Comm::wait()
{
  return this->wait_for(-1);
}

/** @brief Block the calling actor until the communication is finished, or until timeout
 *
 * On timeout, an exception is thrown and the communication is invalidated.
 *
 * @param timeout the amount of seconds to wait for the comm termination.
 *                Negative values denote infinite wait times. 0 as a timeout returns immediately. */
Comm* Comm::wait_for(double timeout)
{
  switch (state_) {
    case State::FINISHED:
      break;

    case State::INITED: // It's not started yet. Do it in one simcall
      if (src_buff_ != nullptr) {
        on_sender_start(*Actor::self());
        simcall_comm_send(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                          copy_data_function_, get_user_data(), timeout);

      } else { // Receiver
        on_receiver_start(*Actor::self());
        simcall_comm_recv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_, copy_data_function_,
                          get_user_data(), timeout, rate_);
      }
      state_ = State::FINISHED;
      break;

    case State::STARTED:
      simcall_comm_wait(pimpl_, timeout);
      on_completion(*Actor::self());
      state_ = State::FINISHED;
      break;

    case State::CANCELED:
      throw CancelException(XBT_THROW_POINT, "Communication canceled");

    default:
      THROW_IMPOSSIBLE;
  }
  return this;
}
int Comm::test_any(std::vector<CommPtr>* comms)
{
  std::unique_ptr<kernel::activity::CommImpl* []> rcomms(new kernel::activity::CommImpl*[comms->size()]);
  std::transform(begin(*comms), end(*comms), rcomms.get(),
                 [](const CommPtr& comm) { return static_cast<kernel::activity::CommImpl*>(comm->pimpl_.get()); });
  return simcall_comm_testany(rcomms.get(), comms->size());
}

Comm* Comm::detach()
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  xbt_assert(src_buff_ != nullptr && src_buff_size_ != 0, "You can only detach sends, not recvs");
  detached_ = true;
  return start();
}

Comm* Comm::cancel()
{
  kernel::actor::simcall([this] {
    if (pimpl_)
      boost::static_pointer_cast<kernel::activity::CommImpl>(pimpl_)->cancel();
  });
  state_ = State::CANCELED;
  return this;
}

bool Comm::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::FINISHED);

  if (state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED)
    this->start();

  if (simcall_comm_test(pimpl_)) {
    state_ = State::FINISHED;
    return true;
  }
  return false;
}

Mailbox* Comm::get_mailbox()
{
  return mailbox_;
}

Actor* Comm::get_sender()
{
  return sender_ ? sender_->ciface() : nullptr;
}

} // namespace s4u
} // namespace simgrid
