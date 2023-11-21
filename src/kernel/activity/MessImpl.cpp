/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/MessImpl.hpp"
#include "src/kernel/activity/MessageQueueImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_mess, kernel, "Kernel message synchronization");

namespace simgrid::kernel::activity {

MessImpl::~MessImpl()
{
  if (queue_)
    queue_->remove(this);
}

MessImpl& MessImpl::set_type(MessImplType type)
{
  type_ = type;
  return *this;
}

MessImpl& MessImpl::set_queue(MessageQueueImpl* queue)
{
  queue_ = queue;
  return *this;
}

MessImpl& MessImpl::set_payload(void* payload)
{
  payload_ = payload;
  return *this;
}

MessImpl& MessImpl::set_dst_buff(unsigned char* buff, size_t* size)
{
  dst_buff_      = buff;
  dst_buff_size_ = size;
  return *this;
}

MessImpl* MessImpl::start()
{
  if (get_state() == State::READY) {
    XBT_DEBUG("Starting message exchange %p from '%s' to '%s' (state: %s)", this, src_actor_->get_host()->get_cname(),
              dst_actor_->get_host()->get_cname(), get_state_str());
    set_state(State::RUNNING);
    finish();
  }
  return this;
}

ActivityImplPtr MessImpl::iput(actor::MessIputSimcall* observer)
{
  auto* queue = observer->get_queue();
  XBT_DEBUG("put from message queue %p", queue);

  /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */
  MessImplPtr this_mess(new MessImpl());
  this_mess->set_type(MessImplType::PUT);

  /* Look for message synchro matching our needs.
   *
   * If it is not found then push our communication into the rendez-vous point */
  MessImplPtr other_mess = queue->find_matching_message(MessImplType::GET);

  if (not other_mess) {
    other_mess = std::move(this_mess);
    queue->push(other_mess);
  } else {
    XBT_DEBUG("Get already pushed");
    other_mess->set_state(State::READY);
  }

  observer->set_message(other_mess.get());
  observer->get_issuer()->activities_.insert(other_mess);

  /* Setup synchro */
  other_mess->src_actor_ = observer->get_issuer();
  other_mess->payload_ = observer->get_payload();
  other_mess->start();

  return other_mess;
}

ActivityImplPtr MessImpl::iget(actor::MessIgetSimcall* observer)
{
  MessImplPtr this_mess(new MessImpl());
  this_mess->set_type(MessImplType::GET);

  auto* queue = observer->get_queue();
  XBT_DEBUG("get from message queue %p. this_synchro=%p", queue, this_mess.get());

  MessImplPtr other_mess = queue->find_matching_message(MessImplType::PUT);

  if (other_mess == nullptr) {
    XBT_DEBUG("Get pushed first (%zu comm enqueued so far)", queue->size());
    other_mess = std::move(this_mess);
    queue->push(other_mess);
  } else {
    XBT_DEBUG("Match my %p with the existing %p", this_mess.get(), other_mess.get());

    other_mess->set_state(State::READY);
  }

  observer->get_issuer()->activities_.insert(other_mess);
  observer->set_message(other_mess.get());

  /* Setup synchro */
  other_mess->set_dst_buff(observer->get_dst_buff(), observer->get_dst_buff_size());
  other_mess->dst_actor_ = observer->get_issuer();

  other_mess->start();

  return other_mess;
}

void MessImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("MessImpl::wait_for(%g), %p, state %s", timeout, this, get_state_str());

  /* Associate this simcall to the wait synchro */
  register_simcall(&issuer->simcall_);
  ActivityImpl::wait_for(issuer, timeout);
}

void MessImpl::cancel()
{
  /* if the synchro is a waiting state means that it is still in a mbox so remove from it and delete it */
  if (get_state() == State::WAITING) {
      queue_->remove(this);
      set_state(State::CANCELED);
  }
}

void MessImpl::finish()
{
  XBT_DEBUG("MessImpl::finish() mess %p, state %s, src_proc %p, dst_proc %p", this, get_state_str(),
            src_actor_.get(), dst_actor_.get());

  if (get_iface()) {
    const auto& piface = static_cast<const s4u::Mess&>(*get_iface());
    set_iface(nullptr); // reset iface to protect against multiple trigger of the on_completion signals
    piface.fire_on_completion_for_real();
    piface.fire_on_this_completion_for_real();
  }

  /* Update synchro state */
  if (get_state() == State::RUNNING) {
    set_state(State::DONE);
  }

  /* If the synchro is still in a rendez-vous point then remove from it */
  if (queue_)
    queue_->remove(this);

  if (get_state() == State::DONE && payload_ != nullptr)
    *(void**)(dst_buff_) = payload_;

  while (not simcalls_.empty()) {
    actor::Simcall* simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == actor::Simcall::Type::NONE) // FIXME: maybe a better way to handle this case
      continue;                                       // if actor handling comm is killed

    handle_activity_waitany(simcall);

    /* Check out for errors */

    if (not simcall->issuer_->get_host()->is_on()) {
      simcall->issuer_->set_wannadie();
    } else {
      // Do not answer to dying actors
      if (not simcall->issuer_->wannadie()) {
        set_exception(simcall->issuer_);
        simcall->issuer_->simcall_answer();
      }
    }

    simcall->issuer_->waiting_synchro_ = nullptr;
    simcall->issuer_->activities_.erase(this);
  }
}

} // namespace simgrid::kernel::activity
