/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Host.hpp>

#define SIMIX_H_NO_DEPRECATED_WARNING // avoid deprecation warning on include (remove with XBT_ATTRIB_DEPRECATED_v333)
#include <simgrid/simix.h>

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_network, kernel, "Kernel network-related synchronization");

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
// XBT_ATTRIB_DEPRECATED_v333
void SIMIX_comm_set_copy_data_callback(void (*callback)(simgrid::kernel::activity::CommImpl*, void*, size_t))
{
  simgrid::kernel::activity::CommImpl::set_copy_data_callback(callback);
}

// XBT_ATTRIB_DEPRECATED_v333
void SIMIX_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  simgrid::s4u::Comm::copy_buffer_callback(comm, buff, buff_size);
}

// XBT_ATTRIB_DEPRECATED_v333
void SIMIX_comm_copy_pointer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  simgrid::s4u::Comm::copy_pointer_callback(comm, buff, buff_size);
}

namespace simgrid {
namespace kernel {
namespace activity {
xbt::signal<void(CommImpl const&)> CommImpl::on_start;
xbt::signal<void(CommImpl const&)> CommImpl::on_completion;

void (*CommImpl::copy_data_callback_)(CommImpl*, void*, size_t) = &s4u::Comm::copy_pointer_callback;

void CommImpl::set_copy_data_callback(void (*callback)(CommImpl*, void*, size_t))
{
  copy_data_callback_ = callback;
}

CommImpl& CommImpl::set_type(CommImplType type)
{
  type_ = type;
  return *this;
}

CommImpl& CommImpl::set_source(s4u::Host* from)
{
  from_ = from;
  return *this;
}

CommImpl& CommImpl::set_destination(s4u::Host* to)
{
  to_ = to;
  return *this;
}

CommImpl& CommImpl::set_size(double size)
{
  size_ = size;
  return *this;
}

CommImpl& CommImpl::set_rate(double rate)
{
  rate_ = rate;
  return *this;
}
CommImpl& CommImpl::set_mailbox(MailboxImpl* mbox)
{
  if (mbox != nullptr)
    mbox_id_ = mbox->get_id();
  mbox_ = mbox;
  return *this;
}

CommImpl& CommImpl::set_src_buff(unsigned char* buff, size_t size)
{
  src_buff_      = buff;
  src_buff_size_ = size;
  return *this;
}

CommImpl& CommImpl::set_dst_buff(unsigned char* buff, size_t* size)
{
  dst_buff_      = buff;
  dst_buff_size_ = size;
  return *this;
}

CommImpl& CommImpl::detach()
{
  detached_ = true;
  return *this;
}

CommImpl::~CommImpl()
{
  XBT_DEBUG("Really free communication %p in state %s (detached = %d)", this, get_state_str(), detached_);

  cleanup_surf();

  if (detached_ && get_state() != State::DONE) {
    /* the communication has failed and was detached:
     * we have to free the buffer */
    if (clean_fun)
      clean_fun(src_buff_);
    src_buff_ = nullptr;
  } else if (mbox_) {
    mbox_->remove(this);
  }
}

/**  @brief Starts the simulation of a communication synchro. */
CommImpl* CommImpl::start()
{
  /* If both the sender and the receiver are already there, start the communication */
  if (get_state() == State::READY) {
    from_ = from_ != nullptr ? from_ : src_actor_->get_host();
    xbt_assert(from_->is_on());
    to_   = to_ != nullptr ? to_ : dst_actor_->get_host();
    xbt_assert(to_->is_on());

    /* Getting the network_model from the origin host
     * Valid while we have a single network model, otherwise we would need to change this function to first get the
     * routes and later create the respective surf actions */
    auto net_model = from_->get_netpoint()->get_englobing_zone()->get_network_model();

    surf_action_ = net_model->communicate(from_, to_, size_, rate_);
    surf_action_->set_activity(this);
    surf_action_->set_category(get_tracing_category());
    set_start_time(surf_action_->get_start_time());
    set_state(State::RUNNING);
    on_start(*this);

    XBT_DEBUG("Starting communication %p from '%s' to '%s' (surf_action: %p; state: %s)", this, from_->get_cname(),
              to_->get_cname(), surf_action_, get_state_str());

    /* If a link is failed, detect it immediately */
    if (surf_action_->get_state() == resource::Action::State::FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure", from_->get_cname(),
                to_->get_cname());
      set_state(State::LINK_FAILURE);
      post();

    } else if ((src_actor_ != nullptr && src_actor_->is_suspended()) ||
               (dst_actor_ != nullptr && dst_actor_->is_suspended())) {
      /* If any of the actor is suspended, create the synchro but stop its execution,
         it will be restarted when the sender actor resume */
      if (src_actor_->is_suspended())
        XBT_DEBUG("The communication is suspended on startup because src (%s@%s) was suspended since it initiated the "
                  "communication",
                  src_actor_->get_cname(), src_actor_->get_host()->get_cname());
      else
        XBT_DEBUG("The communication is suspended on startup because dst (%s@%s) was suspended since it initiated the "
                  "communication",
                  dst_actor_->get_cname(), dst_actor_->get_host()->get_cname());

      surf_action_->suspend();
    }
  }

  return this;
}

std::vector<s4u::Link*> CommImpl::get_traversed_links() const
{
  xbt_assert(get_state() != State::WAITING, "You cannot use %s() if your communication is not ready (%s)", __FUNCTION__,
             get_state_str());
  std::vector<s4u::Link*> vlinks;
  XBT_ATTRIB_UNUSED double res = 0;
  from_->route_to(to_, vlinks, &res);
  return vlinks;
}

/** @brief Copy the communication data from the sender's buffer to the receiver's one  */
void CommImpl::copy_data()
{
  size_t buff_size = src_buff_size_;
  /* If there is no data to copy then return */
  if (not src_buff_ || not dst_buff_ || copied_)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)", this,
            src_actor_ ? src_actor_->get_host()->get_cname() : "a finished actor", src_buff_,
            dst_actor_ ? dst_actor_->get_host()->get_cname() : "a finished actor", dst_buff_, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (dst_buff_size_) {
    buff_size = std::min(buff_size, *dst_buff_size_);

    /* Update the receiver's buffer size to the copied amount */
    *dst_buff_size_ = buff_size;
  }

  if (buff_size > 0) {
    if (copy_data_fun)
      copy_data_fun(this, src_buff_, buff_size);
    else
      copy_data_callback_(this, src_buff_, buff_size);
  }

  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  copied_ = true;
}

ActivityImplPtr CommImpl::isend(actor::CommIsendSimcall* observer)
{
  auto* mbox = observer->get_mailbox();
  XBT_DEBUG("send from mailbox %p", mbox);

  /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */
  CommImplPtr this_comm(new CommImpl());
  this_comm->set_type(CommImplType::SEND);

  /* Look for communication synchro matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  CommImplPtr other_comm =
      mbox->find_matching_comm(CommImplType::RECEIVE, observer->get_match_fun(), observer->get_payload(), this_comm,
                               /*done*/ false, /*remove_matching*/ true);

  if (not other_comm) {
    other_comm = std::move(this_comm);

    if (mbox->is_permanent()) {
      // this mailbox is for small messages, which have to be sent right now
      other_comm->set_state(State::READY);
      other_comm->dst_actor_ = mbox->get_permanent_receiver().get();
      mbox->push_done(other_comm);
      XBT_DEBUG("pushing a message into the permanent receive list %p, comm %p", mbox, other_comm.get());

    } else {
      mbox->push(other_comm);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    other_comm->set_state(State::READY);
  }
  observer->set_comm(other_comm.get());

  if (observer->is_detached()) {
    other_comm->detach();
    other_comm->clean_fun = observer->get_clean_fun();
  } else {
    other_comm->clean_fun = nullptr;
    observer->get_issuer()->activities_.emplace_back(other_comm);
  }

  /* Setup the communication synchro */
  other_comm->src_actor_ = observer->get_issuer();
  other_comm->src_data_  = observer->get_payload();
  (*other_comm)
      .set_src_buff(observer->get_src_buff(), observer->get_src_buff_size())
      .set_size(observer->get_payload_size())
      .set_rate(observer->get_rate());

  other_comm->match_fun     = observer->get_match_fun();
  other_comm->copy_data_fun = observer->get_copy_data_fun();

  if (MC_is_active() || MC_record_replay_is_active())
    other_comm->set_state(simgrid::kernel::activity::State::RUNNING);
  else
    other_comm->start();

  return (observer->is_detached() ? nullptr : other_comm);
}

ActivityImplPtr CommImpl::irecv(actor::CommIrecvSimcall* observer)
{
  CommImplPtr this_synchro(new CommImpl());
  this_synchro->set_type(CommImplType::RECEIVE);

  auto* mbox = observer->get_mailbox();
  XBT_DEBUG("recv from mbox %p. this_synchro=%p", mbox, this_synchro.get());

  CommImplPtr other_comm;
  // communication already done, get it inside the list of completed comms
  if (mbox->is_permanent() && mbox->has_some_done_comm()) {
    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication");
    // find a match in the list of already received comms
    other_comm = mbox->find_matching_comm(CommImplType::SEND, observer->get_match_fun(), observer->get_payload(),
                                          this_synchro, /*done*/ true, /*remove_matching*/ true);
    // if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (not other_comm) {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request "
                "into list");
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      if (other_comm->surf_action_ && other_comm->get_remaining() < 1e-12) {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it", other_comm.get());
        other_comm->set_state(State::DONE);
        other_comm->set_mailbox(nullptr);
      }
    }
  } else {
    /* Prepare a comm describing us, so that it gets passed to the user-provided filter of other side */

    /* Look for communication activity matching our needs. We also provide a description of
     * ourself so that the other side also gets a chance of choosing if it wants to match with us.
     *
     * If it is not found then push our communication into the rendez-vous point */
    other_comm = mbox->find_matching_comm(CommImplType::SEND, observer->get_match_fun(), observer->get_payload(),
                                          this_synchro, /*done*/ false, /*remove_matching*/ true);

    if (other_comm == nullptr) {
      XBT_DEBUG("Receive pushed first (%zu comm enqueued so far)", mbox->size());
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      XBT_DEBUG("Match my %p with the existing %p", this_synchro.get(), other_comm.get());

      other_comm->set_state(simgrid::kernel::activity::State::READY);
    }
    observer->get_issuer()->activities_.emplace_back(other_comm);
  }
  observer->set_comm(other_comm.get());

  /* Setup communication synchro */
  other_comm->dst_actor_ = observer->get_issuer();
  other_comm->dst_data_  = observer->get_payload();
  other_comm->set_dst_buff(observer->get_dst_buff(), observer->get_dst_buff_size());

  if (observer->get_rate() > -1.0 && (other_comm->get_rate() < 0.0 || observer->get_rate() < other_comm->get_rate()))
    other_comm->set_rate(observer->get_rate());

  other_comm->match_fun     = observer->get_match_fun();
  other_comm->copy_data_fun = observer->get_copy_data_fun();

  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->set_state(State::RUNNING);
    return other_comm;
  }
  other_comm->start();

  return other_comm;
}

bool CommImpl::test(actor::ActorImpl* issuer)
{
  if ((MC_is_active() || MC_record_replay_is_active()) && src_actor_ && dst_actor_)
    set_state(State::DONE);
  return ActivityImpl::test(issuer);
}

void CommImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("CommImpl::wait_for(%g), %p, state %s", timeout, this, get_state_str());

  /* Associate this simcall to the wait synchro */
  register_simcall(&issuer->simcall_);
  if (MC_is_active() || MC_record_replay_is_active()) {
    // FIXME: what about timeouts?
    set_state(State::DONE);
    finish();
    return;
  }
  ActivityImpl::wait_for(issuer, timeout);
}

void CommImpl::wait_any_for(actor::ActorImpl* issuer, const std::vector<CommImpl*>& comms, double timeout)
{
  std::vector<ActivityImpl*> activities(comms.begin(), comms.end());
  ActivityImpl::wait_any_for(issuer, activities, timeout);
}

void CommImpl::suspend()
{
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surf_action_)
    surf_action_->suspend();
  /* if not created yet, the action will be suspended on creation, in CommImpl::start() */
}

void CommImpl::resume()
{
  /*FIXME: check what happen with the timeouts */
  if (surf_action_)
    surf_action_->resume();
  /* in the other case, the synchro were not really suspended yet, see CommImpl::suspend() and CommImpl::start() */
}

void CommImpl::cancel()
{
  /* if the synchro is a waiting state means that it is still in a mbox so remove from it and delete it */
  if (get_state() == State::WAITING) {
    if (not detached_) {
      mbox_->remove(this);
      set_state(State::CANCELED);
    }
  } else if (not MC_is_active() /* when running the MC there are no surf actions */
             && not MC_record_replay_is_active() && (get_state() == State::READY || get_state() == State::RUNNING)) {
    surf_action_->cancel();
  }
}

/** @brief This is part of the cleanup process, probably an internal command */
void CommImpl::cleanup_surf()
{
  clean_action();

  if (src_timeout_) {
    src_timeout_->unref();
    src_timeout_ = nullptr;
  }

  if (dst_timeout_) {
    dst_timeout_->unref();
    dst_timeout_ = nullptr;
  }
}

void CommImpl::post()
{
  on_completion(*this);

  /* Update synchro state */
  if (src_timeout_ && src_timeout_->get_state() == resource::Action::State::FINISHED)
    set_state(State::SRC_TIMEOUT);
  else if (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FINISHED)
    set_state(State::DST_TIMEOUT);
  else if ((from_ && not from_->is_on()) || (src_timeout_ && src_timeout_->get_state() == resource::Action::State::FAILED))
    set_state(State::SRC_HOST_FAILURE);
  else if ((to_ && not to_->is_on()) || (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FAILED))
    set_state(State::DST_HOST_FAILURE);
  else if (surf_action_ && surf_action_->get_state() == resource::Action::State::FAILED) {
    set_state(State::LINK_FAILURE);
  } else if (get_state() == State::RUNNING) {
    xbt_assert(from_ && from_->is_on());
    xbt_assert(to_ && to_->is_on());
    set_state(State::DONE);
  }

  XBT_DEBUG("CommImpl::post(): comm %p, state %s, src_proc %p, dst_proc %p, detached: %d", this, get_state_str(),
            src_actor_.get(), dst_actor_.get(), detached_);

  /* destroy the surf actions associated with the Simix communication */
  cleanup_surf();

  /* Answer all simcalls associated with the synchro */
  finish();
}
void CommImpl::set_exception(actor::ActorImpl* issuer)
{
  switch (get_state()) {
    case State::FAILED:
      issuer->exception_ = std::make_exception_ptr(NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
      break;
    case State::SRC_TIMEOUT:
      issuer->exception_ =
          std::make_exception_ptr(TimeoutException(XBT_THROW_POINT, "Communication timeouted because of the sender"));
      break;

    case State::DST_TIMEOUT:
      issuer->exception_ =
          std::make_exception_ptr(TimeoutException(XBT_THROW_POINT, "Communication timeouted because of the receiver"));
      break;

    case State::SRC_HOST_FAILURE:
      if (issuer == src_actor_)
        issuer->context_->set_wannadie();
      else {
        set_state(State::FAILED);
        issuer->exception_ = std::make_exception_ptr(NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
      }
      break;

    case State::DST_HOST_FAILURE:
      if (issuer == dst_actor_)
        issuer->context_->set_wannadie();
      else {
        set_state(State::FAILED);
        issuer->exception_ = std::make_exception_ptr(NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
      }
      break;

    case State::LINK_FAILURE:
      XBT_DEBUG("Link failure in synchro %p between '%s' and '%s': posting an exception to the issuer: %s (%p) "
                "detached:%d",
                this, src_actor_ ? src_actor_->get_host()->get_cname() : nullptr,
                dst_actor_ ? dst_actor_->get_host()->get_cname() : nullptr, issuer->get_cname(), issuer, detached_);
      if (src_actor_ == issuer) {
        XBT_DEBUG("I'm source");
      } else if (dst_actor_ == issuer) {
        XBT_DEBUG("I'm dest");
      } else {
        XBT_DEBUG("I'm neither source nor dest");
      }
      set_state(State::FAILED);
      issuer->throw_exception(std::make_exception_ptr(NetworkFailureException(XBT_THROW_POINT, "Link failure")));
      break;

    case State::CANCELED:
      if (issuer == dst_actor_)
        issuer->exception_ =
            std::make_exception_ptr(CancelException(XBT_THROW_POINT, "Communication canceled by the sender"));
      else
        issuer->exception_ =
            std::make_exception_ptr(CancelException(XBT_THROW_POINT, "Communication canceled by the receiver"));
      break;

    default:
      xbt_assert(get_state() == State::DONE, "Internal error in CommImpl::finish(): unexpected synchro state %s",
                 get_state_str());
  }
}

void CommImpl::finish()
{
  XBT_DEBUG("CommImpl::finish() in state %s", get_state_str());
  /* If the synchro is still in a rendez-vous point then remove from it */
  if (mbox_)
    mbox_->remove(this);

  if (get_state() == State::DONE)
    copy_data();

  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == simix::Simcall::NONE) // FIXME: maybe a better way to handle this case
      continue;                                 // if actor handling comm is killed

    handle_activity_waitany(simcall);

    /* Check out for errors */

    if (not simcall->issuer_->get_host()->is_on()) {
      simcall->issuer_->context_->set_wannadie();
    } else {
      // Do not answer to dying actors
      if (not simcall->issuer_->context_->wannadie()) {
        set_exception(simcall->issuer_);
        simcall->issuer_->simcall_answer();
      }
    }

    simcall->issuer_->waiting_synchro_ = nullptr;
    simcall->issuer_->activities_.remove(this);
    if (detached_) {
      if (simcall->issuer_ != dst_actor_ && dst_actor_ != nullptr)
        dst_actor_->activities_.remove(this);
      if (simcall->issuer_ != src_actor_ && src_actor_ != nullptr)
        src_actor_->activities_.remove(this);
    }
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
