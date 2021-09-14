/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/msg/msg_private.hpp"
#include "xbt/log.h"

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Mailbox.hpp"

#include <simgrid/comm.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm, s4u_activity, "S4U asynchronous communications");

namespace simgrid {
namespace s4u {
xbt::signal<void(Comm const&, bool is_sender)> Comm::on_start;
xbt::signal<void(Comm const&)> Comm::on_completion;

void Comm::complete(Activity::State state)
{
  Activity::complete(state);
  on_completion(*this);
}

Comm::~Comm()
{
  if (state_ == State::STARTED && not detached_ &&
      (pimpl_ == nullptr || pimpl_->state_ == kernel::activity::State::RUNNING)) {
    XBT_INFO("Comm %p freed before its completion. Did you forget to detach it? (state: %s)", this, get_state_str());
    if (pimpl_ != nullptr)
      XBT_INFO("pimpl_->state: %s", pimpl_->get_state_str());
    else
      XBT_INFO("pimpl_ is null");
    xbt_backtrace_display_current();
  }
}

ssize_t Comm::wait_any_for(const std::vector<CommPtr>& comms, double timeout)
{
  std::vector<kernel::activity::CommImpl*> rcomms(comms.size());
  std::transform(begin(comms), end(comms), begin(rcomms),
                 [](const CommPtr& comm) { return static_cast<kernel::activity::CommImpl*>(comm->pimpl_.get()); });
  ssize_t changed_pos = -1;
  try {
    changed_pos = simcall_comm_waitany(rcomms.data(), rcomms.size(), timeout);
  } catch (const NetworkFailureException& e) {
    for (auto c : comms) {
      if (c->pimpl_->state_ == kernel::activity::State::FAILED) {
        c->complete(State::FAILED);
      }
    }
    e.rethrow_nested(XBT_THROW_POINT, boost::core::demangle(typeid(e).name()) + " raised in kernel mode.");
  }
  if (changed_pos != -1)
    comms.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

void Comm::wait_all(const std::vector<CommPtr>& comms)
{
  // TODO: this should be a simcall or something
  for (auto& comm : comms)
    comm->wait();
}

size_t Comm::wait_all_for(const std::vector<CommPtr>& comms, double timeout)
{
  if (timeout < 0.0) {
    wait_all(comms);
    return comms.size();
  }

  double deadline = Engine::get_clock() + timeout;
  std::vector<CommPtr> waited_comm(1, nullptr);
  for (size_t i = 0; i < comms.size(); i++) {
    double wait_timeout = std::max(0.0, deadline - Engine::get_clock());
    waited_comm[0]      = comms[i];
    // Using wait_any_for() here (and not wait_for) because we don't want comms to be invalidated on timeout
    if (wait_any_for(waited_comm, wait_timeout) == -1) {
      XBT_DEBUG("Timeout (%g): i = %zu", wait_timeout, i);
      return i;
    }
  }
  return comms.size();
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
void* Comm::get_dst_data()
{
  return dst_buff_;
}

size_t Comm::get_dst_data_size() const
{
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
CommPtr Comm::set_payload_size(uint64_t bytes)
{
  Activity::set_remaining(bytes);
  return this;
}

CommPtr Comm::sendto_init(Host* from, Host* to)
{
  CommPtr res(new Comm());
  res->from_ = from;
  res->to_   = to;

  return res;
}

CommPtr Comm::sendto_async(Host* from, Host* to, uint64_t simulated_size_in_bytes)
{
  auto res = Comm::sendto_init(from, to)->set_payload_size(simulated_size_in_bytes);
  res->vetoable_start();
  return res;
}

void Comm::sendto(Host* from, Host* to, uint64_t simulated_size_in_bytes)
{
  sendto_async(from, to, simulated_size_in_bytes)->wait();
}

Comm* Comm::start()
{
  xbt_assert(get_state() == State::INITED || get_state() == State::STARTING,
             "You cannot use %s() once your communication started (not implemented)", __FUNCTION__);
  if (from_ != nullptr || to_ != nullptr) {
    xbt_assert(from_ != nullptr && to_ != nullptr, "When either from_ or to_ is specified, both must be.");
    xbt_assert(src_buff_ == nullptr && dst_buff_ == nullptr,
               "Direct host-to-host communications cannot carry any data.");
    pimpl_ = kernel::actor::simcall([this] {
      kernel::activity::CommImplPtr res(new kernel::activity::CommImpl(this->from_, this->to_, this->get_remaining()));
      res->start();
      return res;
    });

  } else if (src_buff_ != nullptr) { // Sender side
    on_start(*this, true /* is_sender*/);
    pimpl_ = simcall_comm_isend(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                                clean_fun_, copy_data_function_, get_user_data(), detached_);
  } else if (dst_buff_ != nullptr) { // Receiver side
    xbt_assert(not detached_, "Receive cannot be detached");
    on_start(*this, false /*is_sender*/);
    pimpl_ = simcall_comm_irecv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_,
                                copy_data_function_, get_user_data(), rate_);

  } else {
    xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
  }

  if (suspended_)
    pimpl_->suspend();

  state_ = State::STARTED;
  return this;
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
    case State::FAILED:
      throw NetworkFailureException(XBT_THROW_POINT, "Cannot wait for a failed communication");

    case State::INITED:
    case State::STARTING: // It's not started yet. Do it in one simcall if it's a regular communication
      if (from_ != nullptr || to_ != nullptr) {
        return vetoable_start()->wait_for(timeout); // In the case of host2host comm, do it in two simcalls
      } else if (src_buff_ != nullptr) {
        on_start(*this, true /*is_sender*/);
        simcall_comm_send(sender_, mailbox_->get_impl(), remains_, rate_, src_buff_, src_buff_size_, match_fun_,
                          copy_data_function_, get_user_data(), timeout);

      } else { // Receiver
        on_start(*this, false /*is_sender*/);
        simcall_comm_recv(receiver_, mailbox_->get_impl(), dst_buff_, &dst_buff_size_, match_fun_, copy_data_function_,
                          get_user_data(), timeout, rate_);
      }
      break;

    case State::STARTED:
      try {
        simcall_comm_wait(get_impl(), timeout);
      } catch (const NetworkFailureException& e) {
        complete(State::FAILED);
        e.rethrow_nested(XBT_THROW_POINT, boost::core::demangle(typeid(e).name()) + " raised in kernel mode.");
      }
      break;

    case State::CANCELED:
      throw CancelException(XBT_THROW_POINT, "Communication canceled");

    default:
      THROW_IMPOSSIBLE;
  }
  complete(State::FINISHED);
  return this;
}

ssize_t Comm::test_any(const std::vector<CommPtr>& comms)
{
  std::vector<kernel::activity::CommImpl*> rcomms(comms.size());
  std::transform(begin(comms), end(comms), begin(rcomms),
                 [](const CommPtr& comm) { return static_cast<kernel::activity::CommImpl*>(comm->pimpl_.get()); });
  ssize_t changed_pos = simcall_comm_testany(rcomms.data(), rcomms.size());
  if (changed_pos != -1)
    comms.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

Comm* Comm::detach()
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication is %s (not implemented)",
             __FUNCTION__, get_state_str());
  xbt_assert(dst_buff_ == nullptr && dst_buff_size_ == 0, "You can only detach sends, not recvs");
  detached_ = true;
  vetoable_start();
  return this;
}

bool Comm::test() // TODO: merge with Activity::test, once modernized
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::STARTING ||
             state_ == State::CANCELED || state_ == State::FINISHED);

  if (state_ == State::CANCELED || state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED || state_ == State::STARTING)
    this->vetoable_start();

  if (simcall_comm_test(get_impl())) {
    complete(State::FINISHED);
    return true;
  }
  return false;
}

Mailbox* Comm::get_mailbox() const
{
  return mailbox_;
}

Actor* Comm::get_sender() const
{
  kernel::actor::ActorImplPtr sender = nullptr;
  if (pimpl_)
    sender = boost::static_pointer_cast<kernel::activity::CommImpl>(pimpl_)->src_actor_;
  return sender ? sender->get_ciface() : nullptr;
}

CommPtr Comm::set_copy_data_callback(void (*callback)(kernel::activity::CommImpl*, void*, size_t))
{
  copy_data_function_ = callback;
  return this;
}

} // namespace s4u
} // namespace simgrid
/* **************************** Public C interface *************************** */
void sg_comm_detach(sg_comm_t comm, void (*clean_function)(void*))
{
  comm->detach(clean_function);
  comm->unref();
}
void sg_comm_unref(sg_comm_t comm)
{
  comm->unref();
}
int sg_comm_test(sg_comm_t comm)
{
  bool finished = comm->test();
  if (finished)
    comm->unref();
  return finished;
}

sg_error_t sg_comm_wait(sg_comm_t comm)
{
  return sg_comm_wait_for(comm, -1);
}

sg_error_t sg_comm_wait_for(sg_comm_t comm, double timeout)
{
  sg_error_t status = SG_OK;

  simgrid::s4u::CommPtr s4u_comm(comm, false);
  try {
    s4u_comm->wait_for(timeout);
  } catch (const simgrid::TimeoutException&) {
    status = SG_ERROR_TIMEOUT;
  } catch (const simgrid::CancelException&) {
    status = SG_ERROR_CANCELED;
  } catch (const simgrid::NetworkFailureException&) {
    status = SG_ERROR_NETWORK;
  }
  return status;
}

void sg_comm_wait_all(sg_comm_t* comms, size_t count)
{
  sg_comm_wait_all_for(comms, count, -1);
}

size_t sg_comm_wait_all_for(sg_comm_t* comms, size_t count, double timeout)
{
  std::vector<simgrid::s4u::CommPtr> s4u_comms;
  for (size_t i = 0; i < count; i++)
    s4u_comms.emplace_back(comms[i], false);

  size_t pos = simgrid::s4u::Comm::wait_all_for(s4u_comms, timeout);
  for (size_t i = pos; i < count; i++)
    s4u_comms[i]->add_ref();
  return pos;
}

ssize_t sg_comm_wait_any(sg_comm_t* comms, size_t count)
{
  return sg_comm_wait_any_for(comms, count, -1);
}

ssize_t sg_comm_wait_any_for(sg_comm_t* comms, size_t count, double timeout)
{
  std::vector<simgrid::s4u::CommPtr> s4u_comms;
  for (size_t i = 0; i < count; i++)
    s4u_comms.emplace_back(comms[i], false);

  ssize_t pos = simgrid::s4u::Comm::wait_any_for(s4u_comms, timeout);
  for (size_t i = 0; i < count; i++) {
    if (pos != -1 && static_cast<size_t>(pos) != i)
      s4u_comms[i]->add_ref();
  }
  return pos;
}
