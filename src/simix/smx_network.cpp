/* Copyright (c) 2009-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix, "SIMIX network-related synchronization");

static void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall);

/******************************************************************************/
/*                          Communication synchros                            */
/******************************************************************************/
XBT_PRIVATE void simcall_HANDLER_comm_send(smx_simcall_t simcall, smx_actor_t src, smx_mailbox_t mbox, double task_size,
                                           double rate, void* src_buff, size_t src_buff_size,
                                           int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                           double timeout)
{
  smx_activity_t comm = simcall_HANDLER_comm_isend(simcall, src, mbox, task_size, rate,
                           src_buff, src_buff_size, match_fun, nullptr, copy_data_fun,
               data, 0);
  SIMCALL_SET_MC_VALUE(simcall, 0);
  simcall_HANDLER_comm_wait(simcall, comm, timeout);
}

XBT_PRIVATE smx_activity_t simcall_HANDLER_comm_isend(
    smx_simcall_t /*simcall*/, smx_actor_t src_proc, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
    size_t src_buff_size, int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
    void (*clean_fun)(void*), // used to free the synchro in case of problem after a detached send
    void (*copy_data_fun)(smx_activity_t, void*, size_t), // used to copy data if not default one
    void* data, int detached)
{
  XBT_DEBUG("send from mailbox %p", mbox);

  /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */
  simgrid::kernel::activity::CommImplPtr this_comm = simgrid::kernel::activity::CommImplPtr(
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

    if (mbox->permanent_receiver_ != nullptr) {
      //this mailbox is for small messages, which have to be sent right now
      other_comm->state_  = SIMIX_READY;
      other_comm->dst_actor_ = mbox->permanent_receiver_.get();
      mbox->done_comm_queue_.push_back(other_comm);
      XBT_DEBUG("pushing a message into the permanent receive list %p, comm %p", mbox, other_comm.get());

    }else{
      mbox->push(other_comm);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    other_comm->state_ = SIMIX_READY;
    other_comm->type   = simgrid::kernel::activity::CommImpl::Type::READY;
  }
  src_proc->comms.push_back(other_comm);

  if (detached) {
    other_comm->detached = true;
    other_comm->clean_fun = clean_fun;
  } else {
    other_comm->clean_fun = nullptr;
  }

  /* Setup the communication synchro */
  other_comm->src_actor_     = src_proc;
  other_comm->task_size_     = task_size;
  other_comm->rate_          = rate;
  other_comm->src_buff_      = src_buff;
  other_comm->src_buff_size_ = src_buff_size;
  other_comm->src_data_      = data;

  other_comm->match_fun = match_fun;
  other_comm->copy_data_fun = copy_data_fun;


  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->state_ = SIMIX_RUNNING;
    return (detached ? nullptr : other_comm);
  }

  other_comm->start();

  return (detached ? nullptr : other_comm);
}

XBT_PRIVATE void simcall_HANDLER_comm_recv(smx_simcall_t simcall, smx_actor_t receiver, smx_mailbox_t mbox,
                                           void* dst_buff, size_t* dst_buff_size,
                                           int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                           double timeout, double rate)
{
  smx_activity_t comm = simcall_HANDLER_comm_irecv(simcall, receiver, mbox, dst_buff, dst_buff_size, match_fun,
                                                   copy_data_fun, data, rate);
  SIMCALL_SET_MC_VALUE(simcall, 0);
  simcall_HANDLER_comm_wait(simcall, comm, timeout);
}

XBT_PRIVATE smx_activity_t simcall_HANDLER_comm_irecv(smx_simcall_t /*simcall*/, smx_actor_t receiver,
                                                      smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                                      simix_match_func_t match_fun,
                                                      void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                                      double rate)
{
  simgrid::kernel::activity::CommImplPtr this_synchro = simgrid::kernel::activity::CommImplPtr(
      new simgrid::kernel::activity::CommImpl(simgrid::kernel::activity::CommImpl::Type::RECEIVE));
  XBT_DEBUG("recv from mbox %p. this_synchro=%p", mbox, this_synchro.get());

  simgrid::kernel::activity::CommImplPtr other_comm;
  //communication already done, get it inside the list of completed comms
  if (mbox->permanent_receiver_ != nullptr && not mbox->done_comm_queue_.empty()) {

    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication");
    //find a match in the list of already received comms
    other_comm = mbox->find_matching_comm(simgrid::kernel::activity::CommImpl::Type::SEND, match_fun, data,
                                          this_synchro, /*done*/ true,
                                          /*remove_matching*/ true);
    //if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (not other_comm) {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request into list");
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      if (other_comm->surf_action_ && other_comm->remains() < 1e-12) {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it", other_comm.get());
        other_comm->state_ = SIMIX_DONE;
        other_comm->type   = simgrid::kernel::activity::CommImpl::Type::DONE;
        other_comm->mbox = nullptr;
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

      other_comm->state_ = SIMIX_READY;
      other_comm->type   = simgrid::kernel::activity::CommImpl::Type::READY;
    }
    receiver->comms.push_back(other_comm);
  }

  /* Setup communication synchro */
  other_comm->dst_actor_     = receiver;
  other_comm->dst_buff_      = dst_buff;
  other_comm->dst_buff_size_ = dst_buff_size;
  other_comm->dst_data_      = data;

  if (rate > -1.0 && (other_comm->rate_ < 0.0 || rate < other_comm->rate_))
    other_comm->rate_ = rate;

  other_comm->match_fun = match_fun;
  other_comm->copy_data_fun = copy_data_fun;

  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->state_ = SIMIX_RUNNING;
    return other_comm;
  }
  other_comm->start();
  return other_comm;
}

void simcall_HANDLER_comm_wait(smx_simcall_t simcall, smx_activity_t synchro, double timeout)
{
  /* Associate this simcall to the wait synchro */
  XBT_DEBUG("simcall_HANDLER_comm_wait, %p", synchro.get());

  synchro->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    if (idx == 0) {
      synchro->state_ = SIMIX_DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout < 0.0)
        THROW_IMPOSSIBLE;

      simgrid::kernel::activity::CommImplPtr comm =
          boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
      if (comm->src_actor_ == simcall->issuer)
        comm->state_ = SIMIX_SRC_TIMEOUT;
      else
        comm->state_ = SIMIX_DST_TIMEOUT;
    }

    SIMIX_comm_finish(synchro);
    return;
  }

  /* If the synchro has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side          */
  if (synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING) {
    SIMIX_comm_finish(synchro);
  } else { /* we need a sleep action (even when there is no timeout) to be notified of host failures */
    simgrid::kernel::resource::Action* sleep = simcall->issuer->host_->pimpl_cpu->sleep(timeout);
    sleep->set_data(synchro.get());

    simgrid::kernel::activity::CommImplPtr comm =
        boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
    if (simcall->issuer == comm->src_actor_)
      comm->src_timeout_ = sleep;
    else
      comm->dst_timeout_ = sleep;
  }
}

void simcall_HANDLER_comm_test(smx_simcall_t simcall, smx_activity_t synchro)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  int res;

  if (MC_is_active() || MC_record_replay_is_active()){
    res = comm->src_actor_ && comm->dst_actor_;
    if (res)
      synchro->state_ = SIMIX_DONE;
  } else {
    res = synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING;
  }

  simcall_comm_test__set__result(simcall, res);
  if (simcall_comm_test__get__result(simcall)) {
    synchro->simcalls_.push_back(simcall);
    SIMIX_comm_finish(synchro);
  } else {
    SIMIX_simcall_answer(simcall);
  }
}

void simcall_HANDLER_comm_testany(smx_simcall_t simcall, simgrid::kernel::activity::ActivityImplPtr comms[],
                                  size_t count)
{
  // The default result is -1 -- this means, "nothing is ready".
  // It can be changed below, but only if something matches.
  simcall_comm_testany__set__result(simcall, -1);

  if (MC_is_active() || MC_record_replay_is_active()){
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    if(idx == -1){
      SIMIX_simcall_answer(simcall);
    }else{
      simgrid::kernel::activity::ActivityImplPtr synchro = comms[idx];
      simcall_comm_testany__set__result(simcall, idx);
      synchro->simcalls_.push_back(simcall);
      synchro->state_ = SIMIX_DONE;
      SIMIX_comm_finish(synchro);
    }
    return;
  }

  for (std::size_t i = 0; i != count; ++i) {
    simgrid::kernel::activity::ActivityImplPtr synchro = comms[i];
    if (synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING) {
      simcall_comm_testany__set__result(simcall, i);
      synchro->simcalls_.push_back(simcall);
      SIMIX_comm_finish(synchro);
      return;
    }
  }
  SIMIX_simcall_answer(simcall);
}

void simcall_HANDLER_comm_waitany(smx_simcall_t simcall, xbt_dynar_t synchros, double timeout)
{
  if (MC_is_active() || MC_record_replay_is_active()){
    if (timeout > 0.0)
      xbt_die("Timeout not implemented for waitany in the model-checker");
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    smx_activity_t synchro = xbt_dynar_get_as(synchros, idx, smx_activity_t);
    synchro->simcalls_.push_back(simcall);
    simcall_comm_waitany__set__result(simcall, idx);
    synchro->state_ = SIMIX_DONE;
    SIMIX_comm_finish(synchro);
    return;
  }

  if (timeout < 0.0){
    simcall->timer = NULL;
  } else {
    simcall->timer = SIMIX_timer_set(SIMIX_get_clock() + timeout, [simcall]() {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      simcall_comm_waitany__set__result(simcall, -1);
      SIMIX_simcall_answer(simcall);
    });
  }

  unsigned int cursor;
  simgrid::kernel::activity::ActivityImpl* ptr;
  xbt_dynar_foreach(synchros, cursor, ptr){
    smx_activity_t synchro = simgrid::kernel::activity::ActivityImplPtr(ptr);
    /* associate this simcall to the the synchro */
    synchro->simcalls_.push_back(simcall);

    /* see if the synchro is already finished */
    if (synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING) {
      SIMIX_comm_finish(synchro);
      break;
    }
  }
}

void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall)
{
  unsigned int cursor = 0;
  xbt_dynar_t synchros = simcall_comm_waitany__get__comms(simcall);

  simgrid::kernel::activity::ActivityImpl* ptr;
  xbt_dynar_foreach(synchros, cursor, ptr){
    smx_activity_t synchro = simgrid::kernel::activity::ActivityImplPtr(ptr);

    // Remove the first occurence of simcall:
    auto i = boost::range::find(synchro->simcalls_, simcall);
    if (i != synchro->simcalls_.end())
      synchro->simcalls_.erase(i);
  }
}

/**
 * @brief Answers the SIMIX simcalls associated to a communication synchro.
 * @param synchro a finished communication synchro
 */
void SIMIX_comm_finish(smx_activity_t synchro)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  while (not synchro->simcalls_.empty()) {
    smx_simcall_t simcall = synchro->simcalls_.front();
    synchro->simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany dynar and return it as the result of the
     * simcall */

    if (simcall->call == SIMCALL_NONE) //FIXME: maybe a better way to handle this case
      continue; // if process handling comm is killed
    if (simcall->call == SIMCALL_COMM_WAITANY) {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      if (simcall->timer) {
        SIMIX_timer_remove(simcall->timer);
        simcall->timer = nullptr;
      }
      if (not MC_is_active() && not MC_record_replay_is_active())
        simcall_comm_waitany__set__result(simcall,
                                          xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &synchro));
    }

    /* If the synchro is still in a rendez-vous point then remove from it */
    if (comm->mbox)
      comm->mbox->remove(comm);

    XBT_DEBUG("SIMIX_comm_finish: synchro state = %d", (int)synchro->state_);

    /* Check out for errors */

    if (not simcall->issuer->host_->is_on()) {
      simcall->issuer->context_->iwannadie = true;
      simcall->issuer->exception_ =
          std::make_exception_ptr(simgrid::HostFailureException(XBT_THROW_POINT, "Host failed"));
    } else {
      switch (comm->state_) {

        case SIMIX_DONE:
          XBT_DEBUG("Communication %p complete!", synchro.get());
          comm->copy_data();
          break;

        case SIMIX_SRC_TIMEOUT:
          simcall->issuer->exception_ = std::make_exception_ptr(
              simgrid::TimeoutError(XBT_THROW_POINT, "Communication timeouted because of the sender"));
          break;

        case SIMIX_DST_TIMEOUT:
          simcall->issuer->exception_ = std::make_exception_ptr(
              simgrid::TimeoutError(XBT_THROW_POINT, "Communication timeouted because of the receiver"));
          break;

        case SIMIX_SRC_HOST_FAILURE:
          if (simcall->issuer == comm->src_actor_)
            simcall->issuer->context_->iwannadie = true;
          else
            simcall->issuer->exception_ =
                std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
          break;

        case SIMIX_DST_HOST_FAILURE:
          if (simcall->issuer == comm->dst_actor_)
            simcall->issuer->context_->iwannadie = true;
          else
            simcall->issuer->exception_ =
                std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Remote peer failed"));
          break;

        case SIMIX_LINK_FAILURE:
          XBT_DEBUG("Link failure in synchro %p between '%s' and '%s': posting an exception to the issuer: %s (%p) "
                    "detached:%d",
                    synchro.get(), comm->src_actor_ ? comm->src_actor_->host_->get_cname() : nullptr,
                    comm->dst_actor_ ? comm->dst_actor_->host_->get_cname() : nullptr, simcall->issuer->get_cname(),
                    simcall->issuer, comm->detached);
          if (comm->src_actor_ == simcall->issuer) {
            XBT_DEBUG("I'm source");
          } else if (comm->dst_actor_ == simcall->issuer) {
            XBT_DEBUG("I'm dest");
          } else {
            XBT_DEBUG("I'm neither source nor dest");
          }
          simcall->issuer->throw_exception(
              std::make_exception_ptr(simgrid::NetworkFailureException(XBT_THROW_POINT, "Link failure")));
          break;

        case SIMIX_CANCELED:
          if (simcall->issuer == comm->dst_actor_)
            simcall->issuer->exception_ = std::make_exception_ptr(
                simgrid::CancelException(XBT_THROW_POINT, "Communication canceled by the sender"));
          else
            simcall->issuer->exception_ = std::make_exception_ptr(
                simgrid::CancelException(XBT_THROW_POINT, "Communication canceled by the receiver"));
          break;

        default:
          xbt_die("Unexpected synchro state in SIMIX_comm_finish: %d", (int)synchro->state_);
      }
    }

    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer->exception_ &&
        (simcall->call == SIMCALL_COMM_WAITANY || simcall->call == SIMCALL_COMM_TESTANY)) {
      // First retrieve the rank of our failing synchro
      int rank = -1;
      if (simcall->call == SIMCALL_COMM_WAITANY) {
        rank = xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &synchro);
      } else if (simcall->call == SIMCALL_COMM_TESTANY) {
        rank         = -1;
        auto* comms  = simcall_comm_testany__get__comms(simcall);
        auto count   = simcall_comm_testany__get__count(simcall);
        auto element = std::find(comms, comms + count, synchro);
        if (element == comms + count)
          rank = -1;
        else
          rank = element - comms;
      }

      // In order to modify the exception we have to rethrow it:
      try {
        std::rethrow_exception(simcall->issuer->exception_);
      } catch (simgrid::TimeoutError& e) {
        e.value                    = rank;
        simcall->issuer->exception_ = std::make_exception_ptr(e);
      } catch (simgrid::NetworkFailureException& e) {
        e.value                    = rank;
        simcall->issuer->exception_ = std::make_exception_ptr(e);
      } catch (simgrid::CancelException& e) {
        e.value                    = rank;
        simcall->issuer->exception_ = std::make_exception_ptr(e);
      }
    }

    simcall->issuer->waiting_synchro = nullptr;
    simcall->issuer->comms.remove(synchro);
    if(comm->detached){
      if (simcall->issuer == comm->src_actor_) {
        if (comm->dst_actor_)
          comm->dst_actor_->comms.remove(synchro);
      } else if (simcall->issuer == comm->dst_actor_) {
        if (comm->src_actor_)
          comm->src_actor_->comms.remove(synchro);
      }
      else{
        comm->dst_actor_->comms.remove(synchro);
        comm->src_actor_->comms.remove(synchro);
      }
    }

    if (simcall->issuer->host_->is_on())
      SIMIX_simcall_answer(simcall);
    else
      simcall->issuer->context_->iwannadie = true;
  }
}

void SIMIX_comm_copy_buffer_callback(smx_activity_t synchro, void* buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  XBT_DEBUG("Copy the data over");
  memcpy(comm->dst_buff_, buff, buff_size);
  if (comm->detached) { // if this is a detached send, the source buffer was duplicated by SMPI sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    comm->src_buff_ = nullptr;
  }
}
