/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm, s4u_activity, "S4U asynchronous communications");

namespace simgrid {
namespace s4u {
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Comm::on_sender_start;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Comm::on_receiver_start;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Comm::on_completion;

Comm::~Comm()
{
  if (state_ == State::STARTED && not detached_ && (pimpl_ == nullptr || pimpl_->state_ == SIMIX_RUNNING)) {
    XBT_INFO("Comm %p freed before its completion. Detached: %d, State: %d", this, detached_, (int)state_);
    if (pimpl_ != nullptr)
      XBT_INFO("pimpl_->state: %d", pimpl_->state_);
    else
      XBT_INFO("pimpl_ is null");
    xbt_backtrace_display_current();
  }
}

int Comm::wait_any_for(std::vector<CommPtr>* comms_in, double timeout)
{
  // Map to dynar<Synchro*>:
  xbt_dynar_t comms = xbt_dynar_new(sizeof(simgrid::kernel::activity::ActivityImpl*), [](void* ptr) {
    intrusive_ptr_release(*(simgrid::kernel::activity::ActivityImpl**)ptr);
  });
  for (auto const& comm : *comms_in) {
    if (comm->state_ == Activity::State::INITED)
      comm->start();
    xbt_assert(comm->state_ == Activity::State::STARTED);
    simgrid::kernel::activity::ActivityImpl* ptr = comm->pimpl_.get();
    intrusive_ptr_add_ref(ptr);
    xbt_dynar_push_as(comms, simgrid::kernel::activity::ActivityImpl*, ptr);
  }
  // Call the underlying simcall:
  int idx = simcall_comm_waitany(comms, timeout);
  xbt_dynar_free(&comms);
  return idx;
}

void Comm::wait_all(std::vector<CommPtr>* comms)
{
  // TODO: this should be a simcall or something
  // TODO: we are missing a version with timeout
  for (CommPtr comm : *comms)
    comm->wait();
}

Comm* Comm::set_rate(double rate)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  rate_ = rate;
  return this;
}

Comm* Comm::set_src_data(void* buff)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  xbt_assert(dst_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  src_buff_ = buff;
  return this;
}
Comm* Comm::set_src_data_size(size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);
  src_buff_size_ = size;
  return this;
}
Comm* Comm::set_src_data(void* buff, size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  xbt_assert(dst_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  src_buff_      = buff;
  src_buff_size_ = size;
  return this;
}
Comm* Comm::set_dst_data(void** buff)
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
Comm* Comm::set_dst_data(void** buff, size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  xbt_assert(src_buff_ == nullptr, "Cannot set the src and dst buffers at the same time");
  dst_buff_      = buff;
  dst_buff_size_ = size;
  return this;
}

Comm* Comm::start()
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __FUNCTION__);

  if (src_buff_ != nullptr) { // Sender side
    on_sender_start(Actor::self());
    pimpl_ = simcall_comm_isend(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                                clean_fun_, copy_data_function_, user_data_, detached_);
  } else if (dst_buff_ != nullptr) { // Receiver side
    xbt_assert(not detached_, "Receive cannot be detached");
    on_receiver_start(Actor::self());
    pimpl_ = simcall_comm_irecv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_,
                                copy_data_function_, user_data_, rate_);

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
 * On timeout, an exception is thrown.
 *
 * @param timeout the amount of seconds to wait for the comm termination.
 *                Negative values denote infinite wait times. 0 as a timeout returns immediately. */
Comm* Comm::wait_for(double timeout)
{
  switch (state_) {
    case State::FINISHED:
      return this;

    case State::INITED: // It's not started yet. Do it in one simcall
      if (src_buff_ != nullptr) {
        on_sender_start(Actor::self());
        simcall_comm_send(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                          copy_data_function_, user_data_, timeout);

      } else { // Receiver
        on_receiver_start(Actor::self());
        simcall_comm_recv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_, copy_data_function_,
                          user_data_, timeout, rate_);
      }
      state_ = State::FINISHED;
      return this;

    case State::STARTED:
      simcall_comm_wait(pimpl_, timeout);
      on_completion(Actor::self());
      state_ = State::FINISHED;
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
  simgrid::simix::simcall([this] { dynamic_cast<kernel::activity::CommImpl*>(pimpl_.get())->cancel(); });
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

MailboxPtr Comm::get_mailbox()
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
} // namespace s4u
} // namespace simgrid
