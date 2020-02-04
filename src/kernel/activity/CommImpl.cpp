/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/CommImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"

#include <boost/range/algorithm.hpp>
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix, "SIMIX network-related synchronization");

XBT_PRIVATE void simcall_HANDLER_comm_send(smx_simcall_t simcall, smx_actor_t src, smx_mailbox_t mbox, double task_size,
                                           double rate, unsigned char* src_buff, size_t src_buff_size,
                                           bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                           void* data, double timeout)
{
  simgrid::kernel::activity::ActivityImplPtr comm = simcall_HANDLER_comm_isend(
      simcall, src, mbox, task_size, rate, src_buff, src_buff_size, match_fun, nullptr, copy_data_fun, data, 0);
  SIMCALL_SET_MC_VALUE(*simcall, 0);
  simcall_HANDLER_comm_wait(simcall, static_cast<simgrid::kernel::activity::CommImpl*>(comm.get()), timeout);
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
  simgrid::kernel::activity::CommImplPtr this_comm =
      simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl());
  this_comm->set_type(simgrid::kernel::activity::CommImpl::Type::SEND);

  /* Look for communication synchro matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  simgrid::kernel::activity::CommImplPtr other_comm =
      mbox->find_matching_comm(simgrid::kernel::activity::CommImpl::Type::RECEIVE, match_fun, data, this_comm,
                               /*done*/ false, /*remove_matching*/ true);

  if (not other_comm) {
    other_comm = std::move(this_comm);

    if (mbox->permanent_receiver_ != nullptr) {
      // this mailbox is for small messages, which have to be sent right now
      other_comm->state_     = simgrid::kernel::activity::State::READY;
      other_comm->dst_actor_ = mbox->permanent_receiver_.get();
      mbox->done_comm_queue_.push_back(other_comm);
      XBT_DEBUG("pushing a message into the permanent receive list %p, comm %p", mbox, other_comm.get());

    } else {
      mbox->push(other_comm);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    other_comm->state_ = simgrid::kernel::activity::State::READY;
    other_comm->set_type(simgrid::kernel::activity::CommImpl::Type::READY);
  }

  if (detached) {
    other_comm->detach();
    other_comm->clean_fun = clean_fun;
  } else {
    other_comm->clean_fun = nullptr;
    src_proc->comms.push_back(other_comm);
  }

  /* Setup the communication synchro */
  other_comm->src_actor_     = src_proc;
  other_comm->src_data_      = data;
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
  SIMCALL_SET_MC_VALUE(*simcall, 0);
  simcall_HANDLER_comm_wait(simcall, static_cast<simgrid::kernel::activity::CommImpl*>(comm.get()), timeout);
}

XBT_PRIVATE simgrid::kernel::activity::ActivityImplPtr
simcall_HANDLER_comm_irecv(smx_simcall_t /*simcall*/, smx_actor_t receiver, smx_mailbox_t mbox, unsigned char* dst_buff,
                           size_t* dst_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                           double rate)
{
  simgrid::kernel::activity::CommImplPtr this_synchro =
      simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl());
  this_synchro->set_type(simgrid::kernel::activity::CommImpl::Type::RECEIVE);
  XBT_DEBUG("recv from mbox %p. this_synchro=%p", mbox, this_synchro.get());

  simgrid::kernel::activity::CommImplPtr other_comm;
  // communication already done, get it inside the list of completed comms
  if (mbox->permanent_receiver_ != nullptr && not mbox->done_comm_queue_.empty()) {
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
        other_comm->set_type(simgrid::kernel::activity::CommImpl::Type::DONE).set_mailbox(nullptr);
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
      XBT_DEBUG("Receive pushed first (%zu comm enqueued so far)", mbox->comm_queue_.size());
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      XBT_DEBUG("Match my %p with the existing %p", this_synchro.get(), other_comm.get());

      other_comm->state_ = simgrid::kernel::activity::State::READY;
      other_comm->set_type(simgrid::kernel::activity::CommImpl::Type::READY);
    }
    receiver->comms.push_back(other_comm);
  }

  /* Setup communication synchro */
  other_comm->dst_actor_     = receiver;
  other_comm->dst_data_      = data;
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
  /* Associate this simcall to the wait synchro */
  XBT_DEBUG("simcall_HANDLER_comm_wait, %p", comm);

  comm->register_simcall(simcall);

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(*simcall);
    if (idx == 0) {
      comm->state_ = simgrid::kernel::activity::State::DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout < 0.0)
        THROW_IMPOSSIBLE;

      if (comm->src_actor_ == simcall->issuer_)
        comm->state_ = simgrid::kernel::activity::State::SRC_TIMEOUT;
      else
        comm->state_ = simgrid::kernel::activity::State::DST_TIMEOUT;
    }

    comm->finish();
    return;
  }

  /* If the synchro has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side          */
  if (comm->state_ != simgrid::kernel::activity::State::WAITING &&
      comm->state_ != simgrid::kernel::activity::State::RUNNING) {
    comm->finish();
  } else { /* we need a sleep action (even when there is no timeout) to be notified of host failures */
    simgrid::kernel::resource::Action* sleep = simcall->issuer_->get_host()->pimpl_cpu->sleep(timeout);
    sleep->set_activity(comm);

    if (simcall->issuer_ == comm->src_actor_)
      comm->src_timeout_ = sleep;
    else
      comm->dst_timeout_ = sleep;
  }
}

void simcall_HANDLER_comm_test(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comm)
{
  bool res;

  if (MC_is_active() || MC_record_replay_is_active()) {
    res = comm->src_actor_ && comm->dst_actor_;
    if (res)
      comm->state_ = simgrid::kernel::activity::State::DONE;
  } else {
    res = comm->state_ != simgrid::kernel::activity::State::WAITING &&
          comm->state_ != simgrid::kernel::activity::State::RUNNING;
  }

  simcall_comm_test__set__result(simcall, res);
  if (res) {
    comm->simcalls_.push_back(simcall);
    comm->finish();
  } else {
    simcall->issuer_->simcall_answer();
  }
}

void simcall_HANDLER_comm_testany(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comms[], size_t count)
{
  // The default result is -1 -- this means, "nothing is ready".
  // It can be changed below, but only if something matches.
  simcall_comm_testany__set__result(simcall, -1);

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(*simcall);
    if (idx == -1) {
      simcall->issuer_->simcall_answer();
    } else {
      simgrid::kernel::activity::CommImpl* comm = comms[idx];
      simcall_comm_testany__set__result(simcall, idx);
      comm->simcalls_.push_back(simcall);
      comm->state_ = simgrid::kernel::activity::State::DONE;
      comm->finish();
    }
    return;
  }

  for (std::size_t i = 0; i != count; ++i) {
    simgrid::kernel::activity::CommImpl* comm = comms[i];
    if (comm->state_ != simgrid::kernel::activity::State::WAITING &&
        comm->state_ != simgrid::kernel::activity::State::RUNNING) {
      simcall_comm_testany__set__result(simcall, i);
      comm->simcalls_.push_back(simcall);
      comm->finish();
      return;
    }
  }
  simcall->issuer_->simcall_answer();
}

static void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall)
{
  simgrid::kernel::activity::CommImpl** comms = simcall_comm_waitany__get__comms(simcall);
  size_t count                                = simcall_comm_waitany__get__count(simcall);

  for (size_t i = 0; i < count; i++) {
    // Remove the first occurrence of simcall:
    auto* comm = comms[i];
    auto j     = boost::range::find(comm->simcalls_, simcall);
    if (j != comm->simcalls_.end())
      comm->simcalls_.erase(j);
  }
}
void simcall_HANDLER_comm_waitany(smx_simcall_t simcall, simgrid::kernel::activity::CommImpl* comms[], size_t count,
                                  double timeout)
{
  if (MC_is_active() || MC_record_replay_is_active()) {
    if (timeout > 0.0)
      xbt_die("Timeout not implemented for waitany in the model-checker");
    int idx                 = SIMCALL_GET_MC_VALUE(*simcall);
    auto* comm              = comms[idx];
    comm->simcalls_.push_back(simcall);
    simcall_comm_waitany__set__result(simcall, idx);
    comm->state_ = simgrid::kernel::activity::State::DONE;
    comm->finish();
    return;
  }

  if (timeout < 0.0) {
    simcall->timeout_cb_ = NULL;
  } else {
    simcall->timeout_cb_ = simgrid::simix::Timer::set(SIMIX_get_clock() + timeout, [simcall]() {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      simcall_comm_waitany__set__result(simcall, -1);
      simcall->issuer_->simcall_answer();
    });
  }

  for (size_t i = 0; i < count; i++) {
    /* associate this simcall to the the synchro */
    auto* comm = comms[i];
    comm->simcalls_.push_back(simcall);

    /* see if the synchro is already finished */
    if (comm->state_ != simgrid::kernel::activity::State::WAITING &&
        comm->state_ != simgrid::kernel::activity::State::RUNNING) {
      comm->finish();
      break;
    }
  }
}

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_comm_copy_data_callback)(simgrid::kernel::activity::CommImpl*, void*,
                                             size_t) = &SIMIX_comm_copy_pointer_callback;

void SIMIX_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  XBT_DEBUG("Copy the data over");
  memcpy(comm->dst_buff_, buff, buff_size);
  if (comm->detached()) { // if this is a detached send, the source buffer was duplicated by SMPI sender to make the
                          // original buffer available to the application ASAP
    xbt_free(buff);
    comm->src_buff_ = nullptr;
  }
}

void SIMIX_comm_set_copy_data_callback(void (*callback)(simgrid::kernel::activity::CommImpl*, void*, size_t))
{
  SIMIX_comm_copy_data_callback = callback;
}

void SIMIX_comm_copy_pointer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size)
{
  xbt_assert((buff_size == sizeof(void*)), "Cannot copy %zu bytes: must be sizeof(void*)", buff_size);
  *(void**)(comm->dst_buff_) = buff;
}

namespace simgrid {
namespace kernel {
namespace activity {

CommImpl& CommImpl::set_type(CommImpl::Type type)
{
  type_ = type;
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
  XBT_DEBUG("Really free communication %p in state %d (detached = %d)", this, static_cast<int>(state_), detached_);

  cleanupSurf();

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
    s4u::Host* sender   = src_actor_->get_host();
    s4u::Host* receiver = dst_actor_->get_host();

    surf_action_ = surf_network_model->communicate(sender, receiver, size_, rate_);
    surf_action_->set_activity(this);
    surf_action_->set_category(get_tracing_category());
    state_ = State::RUNNING;

    XBT_DEBUG("Starting communication %p from '%s' to '%s' (surf_action: %p)", this, sender->get_cname(),
              receiver->get_cname(), surf_action_);

    /* If a link is failed, detect it immediately */
    if (surf_action_->get_state() == resource::Action::State::FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure", sender->get_cname(),
                receiver->get_cname());
      state_ = State::LINK_FAILURE;
      post();

    } else if (src_actor_->is_suspended() || dst_actor_->is_suspended()) {
      /* If any of the process is suspended, create the synchro but stop its execution,
         it will be restarted when the sender process resume */
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

/** @brief Copy the communication data from the sender's buffer to the receiver's one  */
void CommImpl::copy_data()
{
  size_t buff_size = src_buff_size_;
  /* If there is no data to copy then return */
  if (not src_buff_ || not dst_buff_ || copied_)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)", this,
            src_actor_ ? src_actor_->get_host()->get_cname() : "a finished process", src_buff_,
            dst_actor_ ? dst_actor_->get_host()->get_cname() : "a finished process", dst_buff_, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (dst_buff_size_)
    buff_size = std::min(buff_size, *(dst_buff_size_));

  /* Update the receiver's buffer size to the copied amount */
  if (dst_buff_size_)
    *dst_buff_size_ = buff_size;

  if (buff_size > 0) {
    if (copy_data_fun)
      copy_data_fun(this, src_buff_, buff_size);
    else
      SIMIX_comm_copy_data_callback(this, src_buff_, buff_size);
  }

  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  copied_ = true;
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
void CommImpl::cleanupSurf()
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

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d", this, (int)state_,
            src_actor_.get(), dst_actor_.get(), detached_);

  /* destroy the surf actions associated with the Simix communication */
  cleanupSurf();

  /* Answer all simcalls associated with the synchro */
  finish();
}

void CommImpl::finish()
{
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == SIMCALL_NONE) // FIXME: maybe a better way to handle this case
      continue;                         // if actor handling comm is killed
    if (simcall->call_ == SIMCALL_COMM_WAITANY) {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      if (simcall->timeout_cb_) {
        simcall->timeout_cb_->remove();
        simcall->timeout_cb_ = nullptr;
      }
      if (not MC_is_active() && not MC_record_replay_is_active()) {
        CommImpl** comms   = simcall_comm_waitany__get__comms(simcall);
        size_t count       = simcall_comm_waitany__get__count(simcall);
        CommImpl** element = std::find(comms, comms + count, this);
        int rank           = (element != comms + count) ? element - comms : -1;
        simcall_comm_waitany__set__result(simcall, rank);
      }
    }

    /* If the synchro is still in a rendez-vous point then remove from it */
    if (mbox_)
      mbox_->remove(this);

    XBT_DEBUG("CommImpl::finish(): synchro state = %d", static_cast<int>(state_));

    /* Check out for errors */

    if (not simcall->issuer_->get_host()->is_on()) {
      simcall->issuer_->context_->set_wannadie();
    } else {
      switch (state_) {
        case State::DONE:
          XBT_DEBUG("Communication %p complete!", this);
          copy_data();
          break;

        case State::SRC_TIMEOUT:
          simcall->issuer_->exception_ = std::make_exception_ptr(
              simgrid::TimeoutException(XBT_THROW_POINT, "Communication timeouted because of the sender"));
          break;

        case State::DST_TIMEOUT:
          simcall->issuer_->exception_ = std::make_exception_ptr(
              simgrid::TimeoutException(XBT_THROW_POINT, "Communication timeouted because of the receiver"));
          break;

        case State::SRC_HOST_FAILURE:
          if (simcall->issuer_ == src_actor_)
            simcall->issuer_->context_->set_wannadie();
          else
            simcall->issuer_->exception_ =
                std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
          break;

        case State::DST_HOST_FAILURE:
          if (simcall->issuer_ == dst_actor_)
            simcall->issuer_->context_->set_wannadie();
          else
            simcall->issuer_->exception_ =
                std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
          break;

        case State::LINK_FAILURE:
          XBT_DEBUG("Link failure in synchro %p between '%s' and '%s': posting an exception to the issuer: %s (%p) "
                    "detached:%d",
                    this, src_actor_ ? src_actor_->get_host()->get_cname() : nullptr,
                    dst_actor_ ? dst_actor_->get_host()->get_cname() : nullptr, simcall->issuer_->get_cname(),
                    simcall->issuer_, detached_);
          if (src_actor_ == simcall->issuer_) {
            XBT_DEBUG("I'm source");
          } else if (dst_actor_ == simcall->issuer_) {
            XBT_DEBUG("I'm dest");
          } else {
            XBT_DEBUG("I'm neither source nor dest");
          }
          simcall->issuer_->throw_exception(
              std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Link failure")));
          break;

        case State::CANCELED:
          if (simcall->issuer_ == dst_actor_)
            simcall->issuer_->exception_ = std::make_exception_ptr(
                simgrid::CancelException(XBT_THROW_POINT, "Communication canceled by the sender"));
          else
            simcall->issuer_->exception_ = std::make_exception_ptr(
                simgrid::CancelException(XBT_THROW_POINT, "Communication canceled by the receiver"));
          break;

        default:
          xbt_die("Unexpected synchro state in CommImpl::finish: %d", static_cast<int>(state_));
      }
      simcall->issuer_->simcall_answer();
    }
    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer_->exception_ &&
        (simcall->call_ == SIMCALL_COMM_WAITANY || simcall->call_ == SIMCALL_COMM_TESTANY)) {
      // First retrieve the rank of our failing synchro
      CommImpl** comms;
      size_t count;
      if (simcall->call_ == SIMCALL_COMM_WAITANY) {
        comms = simcall_comm_waitany__get__comms(simcall);
        count = simcall_comm_waitany__get__count(simcall);
      } else {
        /* simcall->call_ == SIMCALL_COMM_TESTANY */
        comms = simcall_comm_testany__get__comms(simcall);
        count = simcall_comm_testany__get__count(simcall);
      }
      CommImpl** element = std::find(comms, comms + count, this);
      int rank           = (element != comms + count) ? element - comms : -1;

      // In order to modify the exception we have to rethrow it:
      try {
        std::rethrow_exception(simcall->issuer_->exception_);
      } catch (simgrid::Exception& e) {
        e.value = rank;
      }
    }

    simcall->issuer_->waiting_synchro = nullptr;
    simcall->issuer_->comms.remove(this);
    if (detached_) {
      if (simcall->issuer_ == src_actor_) {
        if (dst_actor_)
          dst_actor_->comms.remove(this);
      } else if (simcall->issuer_ == dst_actor_) {
        if (src_actor_)
          src_actor_->comms.remove(this);
      } else {
        dst_actor_->comms.remove(this);
        src_actor_->comms.remove(this);
      }
    }
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
