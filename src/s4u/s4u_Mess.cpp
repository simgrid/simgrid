/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cmath>
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Mess.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/MessageQueue.hpp>

#include "src/kernel/activity/MessImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_mess, s4u_activity, "S4U asynchronous messaging");

namespace simgrid::s4u {
xbt::signal<void(Mess const&)> Mess::on_send;
xbt::signal<void(Mess const&)> Mess::on_recv;

MessPtr Mess::set_queue(MessageQueue* queue)
{
  queue_ = queue;
  return this;
}

MessPtr Mess::set_payload(void* payload)
{
  payload_ = payload;
  return this;
}

MessPtr Mess::set_dst_data(void** buff, size_t size)
{
  xbt_assert(state_ == State::INITED, "You cannot use %s() once your communication started (not implemented)",
             __func__);

  dst_buff_      = buff;
  dst_buff_size_ = size;
  return this;
}

Actor* Mess::get_sender() const
{
  kernel::actor::ActorImplPtr sender = nullptr;
  if (pimpl_)
    sender = boost::static_pointer_cast<kernel::activity::MessImpl>(pimpl_)->src_actor_;
  return sender ? sender->get_ciface() : nullptr;
}

Actor* Mess::get_receiver() const
{
  kernel::actor::ActorImplPtr receiver = nullptr;
  if (pimpl_)
    receiver = boost::static_pointer_cast<kernel::activity::MessImpl>(pimpl_)->dst_actor_;
  return receiver ? receiver->get_ciface() : nullptr;
}

Mess* Mess::do_start()
{
  xbt_assert(get_state() == State::INITED || get_state() == State::STARTING,
             "You cannot use %s() once your message exchange has started (not implemented)", __func__);

  auto myself = kernel::actor::ActorImpl::self();
  if (myself == sender_) {
    on_send(*this);
    on_this_send(*this);
    kernel::actor::MessIputSimcall observer{sender_, queue_->get_impl(), get_payload()};
    pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iput(&observer); },
                                             &observer);
  } else if (myself == receiver_) {
    on_recv(*this);
    on_this_recv(*this);
    kernel::actor::MessIgetSimcall observer{receiver_,
                                            queue_->get_impl(),
                                            static_cast<unsigned char*>(dst_buff_),
                                            &dst_buff_size_,
                                            get_payload()};
    pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iget(&observer); },
                                             &observer);
  } else {
    xbt_die("Cannot start a message exchange before specifying whether we are the sender or the receiver");
  }

  pimpl_->set_iface(this);
  pimpl_->set_actor(sender_);
  // Only throw the signal when both sides are here and the status is READY
  if (pimpl_->get_state() != kernel::activity::State::WAITING) {
    fire_on_start();
    fire_on_this_start();
  }
  state_ = State::STARTED;
  return this;
}

Mess* Mess::wait_for(double timeout)
{
  XBT_DEBUG("Calling Mess::wait_for with state %s", get_state_str());
  kernel::actor::ActorImpl* issuer = nullptr;
  switch (state_) {
    case State::FINISHED:
      break;
    case State::FAILED:
      throw NetworkFailureException(XBT_THROW_POINT, "Cannot wait for a failed communication");
    case State::INITED:
    case State::STARTING:
      if (get_payload() != nullptr) {
        on_send(*this);
        on_this_send(*this);
        kernel::actor::MessIputSimcall observer{sender_, queue_->get_impl(), get_payload()};
        pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iput(&observer); },
                                                 &observer);
      } else { // Receiver
        on_recv(*this);
        on_this_recv(*this);
        kernel::actor::MessIgetSimcall observer{receiver_,
                                                queue_->get_impl(),
                                                static_cast<unsigned char*>(dst_buff_),
                                                &dst_buff_size_,
                                                get_payload()};
        pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iget(&observer); },
                                                 &observer);
      }
      break;
    case State::STARTED:
      try {
        issuer = kernel::actor::ActorImpl::self();
        kernel::actor::ActivityWaitSimcall observer{issuer, pimpl_.get(), timeout, "Wait"};
        if (kernel::actor::simcall_blocking(
                [&observer] { observer.get_activity()->wait_for(observer.get_issuer(), observer.get_timeout()); },
                &observer)) {
          throw TimeoutException(XBT_THROW_POINT, "Timeouted");
        }
      } catch (const NetworkFailureException& e) {
        issuer->simcall_.observer_ = nullptr; // Comm failed on network failure, reset the observer to nullptr
        complete(State::FAILED);
        e.rethrow_nested(XBT_THROW_POINT, boost::core::demangle(typeid(e).name()) + " raised in kernel mode.");
      }
      break;

    case State::CANCELED:
      throw CancelException(XBT_THROW_POINT, "Message canceled");

    default:
      THROW_IMPOSSIBLE;
  }
  complete(State::FINISHED);
  return this;
}

} // namespace simgrid::s4u
