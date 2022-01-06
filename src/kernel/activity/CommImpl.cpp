/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix, "SIMIX network-related synchronization");

XBT_PRIVATE void simcall_HANDLER_comm_send(smx_simcall_t simcall, smx_actor_t src, smx_mailbox_t mbox, double task_size,
                                           double rate, unsigned char* src_buff, size_t src_buff_size,
                                           bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                           void* data, double timeout)
{
  simgrid::kernel::activity::ActivityImplPtr comm = simcall_HANDLER_comm_isend(
      simcall, src, mbox, task_size, rate, src_buff, src_buff_size, match_fun, nullptr, copy_data_fun, data, false);
  simcall->mc_value_ = 0;
  comm->wait_for(simcall->issuer_, timeout);
}

XBT_PRIVATE simgrid::kernel::activity::ActivityImplPtr simcall_HANDLER_comm_isend(
    smx_simcall_t /*simcall*/, smx_actor_t src_proc, smx_mailbox_t mbox, double task_size, double rate,
    unsigned char* src_buff, size_t src_buff_size,
    bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
    void (*clean_fun)(void*), // used to free the synchro in case of problem after a detached send
    void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), // used to copy data if not default one
    void* data, bool detached)
{
  XBT_DEBUG("send from mailbox %p", mbox);

  /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */
  simgrid::kernel::activity::CommImplPtr this_comm(
      new simgrid::kernel::activity::CommImpl(simgrid::kernel::activity::CommImpl::Type::SEND));

  /* Look for communication synchro matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  simgrid::kernel::activity::CommImplPtr other_comm =
      mbox->find_matching_comm(simgrid::kernel::activity::CommImpl::Type::RECEIVE, match_fun, data, this_comm,
                               /*done*/ false, /*remove_matching*/ true);

  if (not other_comm) {
    other_comm = std::move(this_comm);

    if (mbox->is_permanent()) {
      // this mailbox is for small messages, which have to be sent right now
      other_comm->state_     = simgrid::kernel::activity::State::READY;
      other_comm->dst_actor_ = mbox->get_permanent_receiver().get();
      mbox->push_done(other_comm);
      XBT_DEBUG("pushing a message into the permanent receive list %p, comm %p", mbox, other_comm.get());

    } else {
      mbox->push(other_comm);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    other_comm->state_ = simgrid::kernel::activity::State::READY;
  }

  if (detached) {
    other_comm->detach();
    other_comm->clean_fun = clean_fun;
  } else {
    other_comm->clean_fun = nullptr;
    src_proc->activities_.emplace_back(other_comm);
  }

  /* Setup the communication synchro */
  other_comm->src_actor_ = src_proc;
  other_comm->src_data_  = data;
  (*other_comm).set_src_buff(src_buff, src_buff_size).set_size(task_size).set_rate(rate);

  other_comm->match_fun     = match_fun;
  other_comm->copy_data_fun = copy_data_fun;

  if (MC_is_active() || MC_record_replay_is_active())
    other_comm->state_ = simgrid::kernel::activity::State::RUNNING;
  else
    other_comm->start();

  return (detached ? nullptr : other_comm);
}

XBT_PRIVATE void simcall_HANDLER_comm_recv(smx_simcall_t simcall, smx_actor_t receiver, smx_mailbox_t mbox,
                                           unsigned char* dst_buff, size_t* dst_buff_size,
                                           bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                           void* data, double timeout, double rate)
{
  simgrid::kernel::activity::ActivityImplPtr comm = simcall_HANDLER_comm_irecv(
      simcall, receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
  simcall->mc_value_ = 0;
  comm->wait_for(simcall->issuer_, timeout);
}

XBT_PRIVATE simgrid::kernel::activity::ActivityImplPtr
simcall_HANDLER_comm_irecv(smx_simcall_t /*simcall*/, smx_actor_t receiver, smx_mailbox_t mbox, unsigned char* dst_buff,
                           size_t* dst_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                           double rate)
{
  simgrid::kernel::activity::CommImplPtr this_synchro(
      new simgrid::kernel::activity::CommImpl(simgrid::kernel::activity::CommImpl::Type::RECEIVE));
  XBT_DEBUG("recv from mbox %p. this_synchro=%p", mbox, this_synchro.get());

  simgrid::kernel::activity::CommImplPtr other_comm;
  // communication already done, get it inside the list of completed comms
  if (mbox->is_permanent() && mbox->has_some_done_comm()) {
    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication");
    // find a match in the list of already received comms
    other_comm = mbox->find_matching_comm(simgrid::kernel::activity::CommImpl::Type::SEND, match_fun, data,
                                          this_synchro, /*done*/ true,
                                          /*remove_matching*/ true);
    // if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (not other_comm) {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request "
                "into list");
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      if (other_comm->surf_action_ && other_comm->get_remaining() < 1e-12) {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it", other_comm.get());
        other_comm->state_ = simgrid::kernel::activity::State::DONE;
        other_comm->set_mailbox(nullptr);
      }
    }
  } else {
    /* Prepare a comm describing us, so that it gets passed to the user-provided filter of other side */

    /* Look for communication activity matching our needs. We also provide a description of
     * ourself so that the other side also gets a chance of choosing if it wants to match with us.
     *
     * If it is not found then push our communication into the rendez-vous point */
    other_comm = mbox->find_matching_comm(simgrid::kernel::activity::CommImpl::Type::SEND, match_fun, data,
                                          this_synchro, /*done*/ false,
                                          /*remove_matching*/ true);

    if (other_comm == nullptr) {
      XBT_DEBUG("Receive pushed first (%zu comm enqueued so far)", mbox->size());
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      XBT_DEBUG("Match my %p with the existing %p", this_synchro.get(), other_comm.get());

      other_comm->state_ = simgrid::kernel::activity::State::READY;
    }
    receiver->activities_.emplace_back(other_comm);
  }

  /* Setup communication synchro */
  other_comm->dst_actor_ = receiver;
  other_comm->dst_data_  = data;
  other_comm->set_dst_buff(dst_buff, dst_buff_size);

  if (rate > -1.0 && (other_comm->get_rate() < 0.0 || rate < other_comm->get_rate()))
    other_comm->set_rate(rate);

  other_comm->match_fun     = match_fun;
  other_comm->copy_data_fun = copy_data_fun;

  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->state_ = simgrid::kernel::activity::State::RUNNING;
    return other_comm;
  }
  other_comm->start();
  return other_comm;
}

void simcall_HANDLER_comm_wait(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comm, double timeout)
{
  comm->wait_for(simcall->issuer_, timeout);
}

bool simcall_HANDLER_comm_test(smx_simcall_t, simgrid::kernel::activity::CommImpl* comm)
{
  return comm->test();
}

ssize_t simcall_HANDLER_comm_testany(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comms[], size_t count)
{
  std::vector<simgrid::kernel::activity::CommImpl*> comms_vec(comms, comms + count);
  return simgrid::kernel::activity::CommImpl::test_any(simcall->issuer_, comms_vec);
}

void simcall_HANDLER_comm_waitany(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comms[], size_t count,
                                  double timeout)
{
  std::vector<simgrid::kernel::activity::CommImpl*> comms_vec(comms, comms + count);
  simgrid::kernel::activity::CommImpl::wait_any_for(simcall->issuer_, comms_vec, timeout);
}

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

CommImpl::CommImpl(s4u::Host* from, s4u::Host* to, double bytes) : size_(bytes), detached_(true), from_(from), to_(to)
{
  state_ = State::READY;
}

CommImpl::~CommImpl()
{
  XBT_DEBUG("Really free communication %p in state %s (detached = %d)", this, get_state_str(), detached_);

  cleanup_surf();

  if (detached_ && state_ != State::DONE) {
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
  if (state_ == State::READY) {
    from_ = from_ != nullptr ? from_ : src_actor_->get_host();
    to_   = to_ != nullptr ? to_ : dst_actor_->get_host();
    /* Getting the network_model from the origin host
     * Valid while we have a single network model, otherwise we would need to change this function to first get the
     * routes and later create the respective surf actions */
    auto net_model = from_->get_netpoint()->get_englobing_zone()->get_network_model();

    surf_action_ = net_model->communicate(from_, to_, size_, rate_);
    surf_action_->set_activity(this);
    surf_action_->set_category(get_tracing_category());
    start_time_ = surf_action_->get_start_time();
    state_ = State::RUNNING;
    on_start(*this);

    XBT_DEBUG("Starting communication %p from '%s' to '%s' (surf_action: %p; state: %s)", this, from_->get_cname(),
              to_->get_cname(), surf_action_, get_state_str());

    /* If a link is failed, detect it immediately */
    if (surf_action_->get_state() == resource::Action::State::FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure", from_->get_cname(),
                to_->get_cname());
      state_ = State::LINK_FAILURE;
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
  xbt_assert(state_ != State::WAITING, "You cannot use %s() if your communication is not ready (%s)", __FUNCTION__,
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

bool CommImpl::test()
{
  if ((MC_is_active() || MC_record_replay_is_active()) && src_actor_ && dst_actor_)
    state_ = State::DONE;
  return ActivityImpl::test();
}

void CommImpl::wait_for(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("CommImpl::wait_for(%g), %p, state %s", timeout, this, to_c_str(state_));

  /* Associate this simcall to the wait synchro */
  register_simcall(&issuer->simcall_);

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = issuer->simcall_.mc_value_;
    if (idx == 0) {
      state_ = State::DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout < 0.0)
        THROW_IMPOSSIBLE;
      state_ = (issuer == src_actor_ ? State::SRC_TIMEOUT : State::DST_TIMEOUT);
    }
    finish();
    return;
  }

  /* If the synchro has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side          */
  if (state_ != State::WAITING && state_ != State::RUNNING) {
    finish();
  } else { /* we need a sleep action (even when there is no timeout) to be notified of host failures */
    resource::Action* sleep = issuer->get_host()->get_cpu()->sleep(timeout);
    sleep->set_activity(this);

    if (issuer == src_actor_)
      src_timeout_ = sleep;
    else
      dst_timeout_ = sleep;
  }
}

ssize_t CommImpl::test_any(const actor::ActorImpl* issuer, const std::vector<CommImpl*>& comms)
{
  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = issuer->simcall_.mc_value_;
    xbt_assert(idx == -1 || comms[idx]->test());
    return idx;
  }

  for (std::size_t i = 0; i < comms.size(); ++i) {
    if (comms[i]->test())
      return i;
  }
  return -1;
}

void CommImpl::wait_any_for(actor::ActorImpl* issuer, const std::vector<CommImpl*>& comms, double timeout)
{
  if (MC_is_active() || MC_record_replay_is_active()) {
    xbt_assert(timeout <= 0.0, "Timeout not implemented for waitany in the model-checker");
    int idx    = issuer->simcall_.mc_value_;
    auto* comm = comms[idx];
    comm->simcalls_.push_back(&issuer->simcall_);
    simcall_comm_waitany__set__result(&issuer->simcall_, idx);
    comm->state_ = State::DONE;
    comm->finish();
    return;
  }

  if (timeout < 0.0) {
    issuer->simcall_.timeout_cb_ = nullptr;
  } else {
    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [issuer, comms]() {
      // FIXME: Vector `comms' is copied here. Use a reference once its lifetime is extended (i.e. when the simcall is
      // modernized).
      issuer->simcall_.timeout_cb_ = nullptr;
      for (auto* comm : comms)
        comm->unregister_simcall(&issuer->simcall_);
      simcall_comm_waitany__set__result(&issuer->simcall_, -1);
      issuer->simcall_answer();
    });
  }

  for (auto* comm : comms) {
    /* associate this simcall to the the synchro */
    comm->simcalls_.push_back(&issuer->simcall_);

    /* see if the synchro is already finished */
    if (comm->state_ != State::WAITING && comm->state_ != State::RUNNING) {
      comm->finish();
      break;
    }
  }
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
  if (state_ == State::WAITING) {
    if (not detached_) {
      mbox_->remove(this);
      state_ = State::CANCELED;
    }
  } else if (not MC_is_active() /* when running the MC there are no surf actions */
             && not MC_record_replay_is_active() && (state_ == State::READY || state_ == State::RUNNING)) {
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
    state_ = State::SRC_TIMEOUT;
  else if (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FINISHED)
    state_ = State::DST_TIMEOUT;
  else if (src_timeout_ && src_timeout_->get_state() == resource::Action::State::FAILED)
    state_ = State::SRC_HOST_FAILURE;
  else if (dst_timeout_ && dst_timeout_->get_state() == resource::Action::State::FAILED)
    state_ = State::DST_HOST_FAILURE;
  else if (surf_action_ && surf_action_->get_state() == resource::Action::State::FAILED) {
    state_ = State::LINK_FAILURE;
  } else
    state_ = State::DONE;

  XBT_DEBUG("CommImpl::post(): comm %p, state %s, src_proc %p, dst_proc %p, detached: %d", this, get_state_str(),
            src_actor_.get(), dst_actor_.get(), detached_);

  /* destroy the surf actions associated with the Simix communication */
  cleanup_surf();

  /* Answer all simcalls associated with the synchro */
  finish();
}
void CommImpl::set_exception(actor::ActorImpl* issuer)
{
  switch (state_) {
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
        state_             = kernel::activity::State::FAILED;
        issuer->exception_ = std::make_exception_ptr(NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
      }
      break;

    case State::DST_HOST_FAILURE:
      if (issuer == dst_actor_)
        issuer->context_->set_wannadie();
      else {
        state_             = kernel::activity::State::FAILED;
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
      state_ = kernel::activity::State::FAILED;
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
      xbt_assert(state_ == State::DONE, "Internal error in CommImpl::finish(): unexpected synchro state %s",
                 to_c_str(state_));
  }
}

void CommImpl::finish()
{
  XBT_DEBUG("CommImpl::finish() in state %s", to_c_str(state_));
  /* If the synchro is still in a rendez-vous point then remove from it */
  if (mbox_)
    mbox_->remove(this);

  if (state_ == State::DONE)
    copy_data();

  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == simix::Simcall::NONE) // FIXME: maybe a better way to handle this case
      continue;                                 // if actor handling comm is killed
    if (simcall->call_ == simix::Simcall::COMM_WAITANY) {
      CommImpl** comms = simcall_comm_waitany__get__comms(simcall);
      size_t count     = simcall_comm_waitany__get__count(simcall);
      for (size_t i = 0; i < count; i++)
        comms[i]->unregister_simcall(simcall);
      if (simcall->timeout_cb_) {
        simcall->timeout_cb_->remove();
        simcall->timeout_cb_ = nullptr;
      }
      if (not MC_is_active() && not MC_record_replay_is_active()) {
        auto element = std::find(comms, comms + count, this);
        ssize_t rank = (element != comms + count) ? element - comms : -1;
        simcall_comm_waitany__set__result(simcall, rank);
      }
    }

    /* Check out for errors */

    if (not simcall->issuer_->get_host()->is_on()) {
      simcall->issuer_->context_->set_wannadie();
    } else {
      set_exception(simcall->issuer_);
      simcall->issuer_->simcall_answer();
    }
    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer_->exception_ &&
        (simcall->call_ == simix::Simcall::COMM_WAITANY || simcall->call_ == simix::Simcall::COMM_TESTANY)) {
      // First retrieve the rank of our failing synchro
      CommImpl** comms;
      size_t count;
      if (simcall->call_ == simix::Simcall::COMM_WAITANY) {
        comms = simcall_comm_waitany__get__comms(simcall);
        count = simcall_comm_waitany__get__count(simcall);
      } else {
        /* simcall->call_ == simix::Simcall::COMM_TESTANY */
        comms = simcall_comm_testany__get__comms(simcall);
        count = simcall_comm_testany__get__count(simcall);
      }
      auto element = std::find(comms, comms + count, this);
      ssize_t rank = (element != comms + count) ? element - comms : -1;
      // In order to modify the exception we have to rethrow it:
      try {
        std::rethrow_exception(simcall->issuer_->exception_);
      } catch (Exception& e) {
        e.set_value(rank);
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
