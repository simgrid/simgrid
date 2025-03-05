/* Copyright (c) 2023-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Mess.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/MessageQueue.hpp>

#include "src/kernel/activity/MessImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/WaitTestObserver.hpp"

#include <boost/core/demangle.hpp>
#include <cmath>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_mess, s4u_activity, "S4U asynchronous messaging");

namespace simgrid::s4u {
xbt::signal<void(Mess const&)> Mess::on_send;
xbt::signal<void(Mess const&)> Mess::on_recv;

template <> xbt::signal<void(Mess&)> Activity_T<Mess>::on_veto             = xbt::signal<void(Mess&)>();
template <> xbt::signal<void(Mess const&)> Activity_T<Mess>::on_start      = xbt::signal<void(Mess const&)>();
template <> xbt::signal<void(Mess const&)> Activity_T<Mess>::on_completion = xbt::signal<void(Mess const&)>();
template <> xbt::signal<void(Mess const&)> Activity_T<Mess>::on_suspend    = xbt::signal<void(Mess const&)>();
template <> xbt::signal<void(Mess const&)> Activity_T<Mess>::on_resume     = xbt::signal<void(Mess const&)>();
template <> void Activity_T<Mess>::fire_on_start() const
{
  on_start(static_cast<const Mess&>(*this));
}
template <> void Activity_T<Mess>::fire_on_completion() const
{
  on_completion(static_cast<const Mess&>(*this));
}
template <> void Activity_T<Mess>::fire_on_suspend() const
{
  on_suspend(static_cast<const Mess&>(*this));
}
template <> void Activity_T<Mess>::fire_on_resume() const
{
  on_resume(static_cast<const Mess&>(*this));
}
template <> void Activity_T<Mess>::fire_on_veto()
{
  on_veto(static_cast<Mess&>(*this));
}
template <> void Activity_T<Mess>::on_start_cb(const std::function<void(Mess const&)>& cb)
{
  on_start.connect(cb);
}
template <> void Activity_T<Mess>::on_completion_cb(const std::function<void(Mess const&)>& cb)
{
  on_completion.connect(cb);
}
template <> void Activity_T<Mess>::on_suspend_cb(const std::function<void(Mess const&)>& cb)
{
  on_suspend.connect(cb);
}
template <> void Activity_T<Mess>::on_resume_cb(const std::function<void(Mess const&)>& cb)
{
  on_resume.connect(cb);
}
template <> void Activity_T<Mess>::on_veto_cb(const std::function<void(Mess&)>& cb)
{
  on_veto.connect(cb);
}
void Mess::fire_on_completion_for_real() const
{
  Activity_T<Mess>::fire_on_completion();
}
void Mess::fire_on_this_completion_for_real() const
{
  Activity_T<Mess>::fire_on_this_completion();
}

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
    kernel::actor::MessIputSimcall observer{sender_, queue_->get_impl(), get_clean_function(), payload_, detached_};
    pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iput(&observer); },
                                             &observer);
  } else if (myself == receiver_) {
    on_recv(*this);
    on_this_recv(*this);
    kernel::actor::MessIgetSimcall observer{receiver_,
                                            queue_->get_impl(),
                                            static_cast<unsigned char*>(dst_buff_),
                                            &dst_buff_size_,
                                            payload_};
    pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iget(&observer); },
                                             &observer);
  } else {
    xbt_die("Cannot start a message exchange before specifying whether we are the sender or the receiver");
  }

  if (not detached_) {
    pimpl_->set_iface(this);
    pimpl_->set_actor(sender_);
    // Only throw the signal when both sides are here and the status is READY
    if (pimpl_->get_state() != kernel::activity::State::WAITING) {
      fire_on_start();
      fire_on_this_start();
    }
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
      if (payload_ != nullptr) {
        on_send(*this);
        on_this_send(*this);
        kernel::actor::MessIputSimcall observer{sender_, queue_->get_impl(), get_clean_function(), payload_, detached_};
        pimpl_ = kernel::actor::simcall_answered([&observer] { return kernel::activity::MessImpl::iput(&observer); },
                                                 &observer);
      } else { // Receiver
        on_recv(*this);
        on_this_recv(*this);
        kernel::actor::MessIgetSimcall observer{receiver_,
                                                queue_->get_impl(),
                                                static_cast<unsigned char*>(dst_buff_),
                                                &dst_buff_size_,
                                                payload_};
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

void* Mess::get_payload() const
{
  xbt_assert(get_state() == State::FINISHED,
             "You can only retrieve the payload of a Message that gracefully terminated, but its state is %s.",
             get_state_str());
  return static_cast<kernel::activity::MessImpl*>(pimpl_.get())->get_payload();
}

} // namespace simgrid::s4u
