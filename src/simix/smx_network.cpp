/* Copyright (c) 2009-2017. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include <boost/range/algorithm.hpp>

#include "src/kernel/activity/CommImpl.hpp"
#include <xbt/ex.hpp>

#include "simgrid/s4u/Host.hpp"

#include "mc/mc.h"
#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

#include "src/surf/network_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix, "SIMIX network-related synchronization");

static void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall);
static void SIMIX_comm_copy_data(smx_activity_t comm);
static void SIMIX_comm_start(simgrid::kernel::activity::CommImplPtr synchro);

/**
 *  \brief Checks if there is a communication activity queued in a deque matching our needs
 *  \param deque where to search into
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \param match_fun the function to apply
 *  \param this_user_data additional parameter to the match_fun
 *  \param my_synchro what to compare against
 *  \param remove_matching whether or not to clean the found object from the queue
 *  \return The communication activity if found, nullptr otherwise
 */
static simgrid::kernel::activity::CommImplPtr
_find_matching_comm(boost::circular_buffer_space_optimized<smx_activity_t>* deque, e_smx_comm_type_t type,
                    int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*), void* this_user_data,
                    simgrid::kernel::activity::CommImplPtr my_synchro, bool remove_matching)
{
  void* other_user_data = nullptr;

  for(auto it = deque->begin(); it != deque->end(); it++){
    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(std::move(*it));

    if (comm->type == SIMIX_COMM_SEND) {
      other_user_data = comm->src_data;
    } else if (comm->type == SIMIX_COMM_RECEIVE) {
      other_user_data = comm->dst_data;
    }
    if (comm->type == type && (match_fun == nullptr || match_fun(this_user_data, other_user_data, comm.get())) &&
        (not comm->match_fun || comm->match_fun(other_user_data, this_user_data, my_synchro.get()))) {
      XBT_DEBUG("Found a matching communication synchro %p", comm.get());
      if (remove_matching)
        deque->erase(it);
#if SIMGRID_HAVE_MC
      comm->mbox_cpy = comm->mbox;
#endif
      comm->mbox = nullptr;
      return comm;
    }
    XBT_DEBUG("Sorry, communication synchro %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              comm.get(), (int)comm->type, (int)type);
  }
  XBT_DEBUG("No matching communication synchro found");
  return nullptr;
}

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
  simgrid::kernel::activity::CommImplPtr this_comm =
      simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl(SIMIX_COMM_SEND));

  /* Look for communication synchro matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  simgrid::kernel::activity::CommImplPtr other_comm =
      _find_matching_comm(&mbox->comm_queue, SIMIX_COMM_RECEIVE, match_fun, data, this_comm, /*remove_matching*/ true);

  if (not other_comm) {
    other_comm = std::move(this_comm);

    if (mbox->permanent_receiver != nullptr) {
      //this mailbox is for small messages, which have to be sent right now
      other_comm->state   = SIMIX_READY;
      other_comm->dst_proc=mbox->permanent_receiver.get();
      mbox->done_comm_queue.push_back(other_comm);
      XBT_DEBUG("pushing a message into the permanent receive list %p, comm %p", mbox, other_comm.get());

    }else{
      mbox->push(other_comm);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    other_comm->state = SIMIX_READY;
    other_comm->type = SIMIX_COMM_READY;
  }
  src_proc->comms.push_back(other_comm);

  if (detached) {
    other_comm->detached = true;
    other_comm->clean_fun = clean_fun;
  } else {
    other_comm->clean_fun = nullptr;
  }

  /* Setup the communication synchro */
  other_comm->src_proc = src_proc;
  other_comm->task_size = task_size;
  other_comm->rate = rate;
  other_comm->src_buff = src_buff;
  other_comm->src_buff_size = src_buff_size;
  other_comm->src_data = data;

  other_comm->match_fun = match_fun;
  other_comm->copy_data_fun = copy_data_fun;


  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->state = SIMIX_RUNNING;
    return (detached ? nullptr : other_comm);
  }

  SIMIX_comm_start(other_comm);
  return (detached ? nullptr : other_comm);
}

XBT_PRIVATE void simcall_HANDLER_comm_recv(smx_simcall_t simcall, smx_actor_t receiver, smx_mailbox_t mbox,
                                           void* dst_buff, size_t* dst_buff_size,
                                           int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                           void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                           double timeout, double rate)
{
  smx_activity_t comm = SIMIX_comm_irecv(receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
  SIMCALL_SET_MC_VALUE(simcall, 0);
  simcall_HANDLER_comm_wait(simcall, comm, timeout);
}

XBT_PRIVATE smx_activity_t simcall_HANDLER_comm_irecv(smx_simcall_t /*simcall*/, smx_actor_t receiver,
                                                      smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                                      simix_match_func_t match_fun,
                                                      void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                                      double rate)
{
  return SIMIX_comm_irecv(receiver, mbox, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
}

smx_activity_t
SIMIX_comm_irecv(smx_actor_t dst_proc, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                 int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                 void (*copy_data_fun)(smx_activity_t, void*, size_t), // used to copy data if not default one
                 void* data, double rate)
{
  simgrid::kernel::activity::CommImplPtr this_synchro =
      simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl(SIMIX_COMM_RECEIVE));
  XBT_DEBUG("recv from mbox %p. this_synchro=%p", mbox, this_synchro.get());

  simgrid::kernel::activity::CommImplPtr other_comm;
  //communication already done, get it inside the list of completed comms
  if (mbox->permanent_receiver != nullptr && not mbox->done_comm_queue.empty()) {

    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication");
    //find a match in the list of already received comms
    other_comm = _find_matching_comm(&mbox->done_comm_queue, SIMIX_COMM_SEND, match_fun, data, this_synchro,
                                     /*remove_matching*/ true);
    //if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (not other_comm) {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request into list");
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      if (other_comm->surfAction_ && other_comm->remains() < 1e-12) {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it", other_comm.get());
        other_comm->state = SIMIX_DONE;
        other_comm->type = SIMIX_COMM_DONE;
        other_comm->mbox = nullptr;
      }
    }
  } else {
    /* Prepare a comm describing us, so that it gets passed to the user-provided filter of other side */

    /* Look for communication activity matching our needs. We also provide a description of
     * ourself so that the other side also gets a chance of choosing if it wants to match with us.
     *
     * If it is not found then push our communication into the rendez-vous point */
    other_comm = _find_matching_comm(&mbox->comm_queue, SIMIX_COMM_SEND, match_fun, data, this_synchro,
                                     /*remove_matching*/ true);

    if (other_comm == nullptr) {
      XBT_DEBUG("Receive pushed first (%zu comm enqueued so far)", mbox->comm_queue.size());
      other_comm = std::move(this_synchro);
      mbox->push(other_comm);
    } else {
      XBT_DEBUG("Match my %p with the existing %p", this_synchro.get(), other_comm.get());

      other_comm->state = SIMIX_READY;
      other_comm->type = SIMIX_COMM_READY;
    }
    dst_proc->comms.push_back(other_comm);
  }

  /* Setup communication synchro */
  other_comm->dst_proc = dst_proc;
  other_comm->dst_buff = dst_buff;
  other_comm->dst_buff_size = dst_buff_size;
  other_comm->dst_data = data;

  if (rate > -1.0 && (other_comm->rate < 0.0 || rate < other_comm->rate))
    other_comm->rate = rate;

  other_comm->match_fun = match_fun;
  other_comm->copy_data_fun = copy_data_fun;

  if (MC_is_active() || MC_record_replay_is_active()) {
    other_comm->state = SIMIX_RUNNING;
    return other_comm;
  }

  SIMIX_comm_start(other_comm);
  return other_comm;
}

smx_activity_t simcall_HANDLER_comm_iprobe(smx_simcall_t simcall, smx_mailbox_t mbox, int type,
                                           simix_match_func_t match_fun, void* data)
{
  return SIMIX_comm_iprobe(simcall->issuer, mbox, type, match_fun, data);
}

smx_activity_t SIMIX_comm_iprobe(smx_actor_t dst_proc, smx_mailbox_t mbox, int type, simix_match_func_t match_fun,
                                 void* data)
{
  XBT_DEBUG("iprobe from %p %p", mbox, &mbox->comm_queue);
  simgrid::kernel::activity::CommImplPtr this_comm;
  int smx_type;
  if(type == 1){
    this_comm = simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl(SIMIX_COMM_SEND));
    smx_type = SIMIX_COMM_RECEIVE;
  } else{
    this_comm = simgrid::kernel::activity::CommImplPtr(new simgrid::kernel::activity::CommImpl(SIMIX_COMM_RECEIVE));
    smx_type = SIMIX_COMM_SEND;
  }
  smx_activity_t other_synchro=nullptr;
  if (mbox->permanent_receiver != nullptr && not mbox->done_comm_queue.empty()) {
    XBT_DEBUG("first check in the permanent recv mailbox, to see if we already got something");
    other_synchro = _find_matching_comm(&mbox->done_comm_queue,
      (e_smx_comm_type_t) smx_type, match_fun, data, this_comm,/*remove_matching*/false);
  }
  if (not other_synchro) {
    XBT_DEBUG("check if we have more luck in the normal mailbox");
    other_synchro = _find_matching_comm(&mbox->comm_queue,
      (e_smx_comm_type_t) smx_type, match_fun, data, this_comm,/*remove_matching*/false);
  }

  return other_synchro;
}

void simcall_HANDLER_comm_wait(smx_simcall_t simcall, smx_activity_t synchro, double timeout)
{
  /* Associate this simcall to the wait synchro */
  XBT_DEBUG("simcall_HANDLER_comm_wait, %p", synchro.get());

  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    if (idx == 0) {
      synchro->state = SIMIX_DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout < 0.0)
        THROW_IMPOSSIBLE;

      simgrid::kernel::activity::CommImplPtr comm =
          boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
      if (comm->src_proc == simcall->issuer)
        comm->state = SIMIX_SRC_TIMEOUT;
      else
        comm->state = SIMIX_DST_TIMEOUT;
    }

    SIMIX_comm_finish(synchro);
    return;
  }

  /* If the synchro has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side          */
  if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING) {
    SIMIX_comm_finish(synchro);
  } else { /* we need a surf sleep action even when there is no timeout, otherwise surf won't tell us when the host
              fails */
    surf_action_t sleep = simcall->issuer->host->pimpl_cpu->sleep(timeout);
    sleep->setData(synchro.get());

    simgrid::kernel::activity::CommImplPtr comm =
        boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
    if (simcall->issuer == comm->src_proc)
      comm->src_timeout = sleep;
    else
      comm->dst_timeout = sleep;
  }
}

void simcall_HANDLER_comm_test(smx_simcall_t simcall, smx_activity_t synchro)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  if (MC_is_active() || MC_record_replay_is_active()){
    simcall_comm_test__set__result(simcall, comm->src_proc && comm->dst_proc);
    if (simcall_comm_test__get__result(simcall)){
      synchro->state = SIMIX_DONE;
      synchro->simcalls.push_back(simcall);
      SIMIX_comm_finish(synchro);
    } else {
      SIMIX_simcall_answer(simcall);
    }
    return;
  }

  simcall_comm_test__set__result(simcall, (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING));
  if (simcall_comm_test__get__result(simcall)) {
    synchro->simcalls.push_back(simcall);
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
      synchro->simcalls.push_back(simcall);
      synchro->state = SIMIX_DONE;
      SIMIX_comm_finish(synchro);
    }
    return;
  }

  for (std::size_t i = 0; i != count; ++i) {
    simgrid::kernel::activity::ActivityImplPtr synchro = comms[i];
    if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING) {
      simcall_comm_testany__set__result(simcall, i);
      synchro->simcalls.push_back(simcall);
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
    synchro->simcalls.push_back(simcall);
    simcall_comm_waitany__set__result(simcall, idx);
    synchro->state = SIMIX_DONE;
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
    synchro->simcalls.push_back(simcall);

    /* see if the synchro is already finished */
    if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING){
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
    auto i = boost::range::find(synchro->simcalls, simcall);
    if (i !=  synchro->simcalls.end())
      synchro->simcalls.erase(i);
  }
}

/**
 *  \brief Starts the simulation of a communication synchro.
 *  \param comm the communication that will be started
 */
static inline void SIMIX_comm_start(simgrid::kernel::activity::CommImplPtr comm)
{
  /* If both the sender and the receiver are already there, start the communication */
  if (comm->state == SIMIX_READY) {

    simgrid::s4u::Host* sender   = comm->src_proc->host;
    simgrid::s4u::Host* receiver = comm->dst_proc->host;

    comm->surfAction_ = surf_network_model->communicate(sender, receiver, comm->task_size, comm->rate);
    comm->surfAction_->setData(comm.get());
    comm->state = SIMIX_RUNNING;

    XBT_DEBUG("Starting communication %p from '%s' to '%s' (surf_action: %p)", comm.get(), sender->getCname(),
              receiver->getCname(), comm->surfAction_);

    /* If a link is failed, detect it immediately */
    if (comm->surfAction_->getState() == simgrid::surf::Action::State::failed) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure", sender->getCname(),
                receiver->getCname());
      comm->state = SIMIX_LINK_FAILURE;
      comm->cleanupSurf();
    }

    /* If any of the process is suspend, create the synchro but stop its execution,
       it will be restarted when the sender process resume */
    if (comm->src_proc->isSuspended() || comm->dst_proc->isSuspended()) {
      if (comm->src_proc->isSuspended())
        XBT_DEBUG("The communication is suspended on startup because src (%s@%s) was suspended since it initiated the "
                  "communication",
                  comm->src_proc->getCname(), comm->src_proc->host->getCname());
      else
        XBT_DEBUG("The communication is suspended on startup because dst (%s@%s) was suspended since it initiated the "
                  "communication",
                  comm->dst_proc->getCname(), comm->dst_proc->host->getCname());

      comm->surfAction_->suspend();
    }
  }
}

/**
 * \brief Answers the SIMIX simcalls associated to a communication synchro.
 * \param synchro a finished communication synchro
 */
void SIMIX_comm_finish(smx_activity_t synchro)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  while (not synchro->simcalls.empty()) {
    smx_simcall_t simcall = synchro->simcalls.front();
    synchro->simcalls.pop_front();

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

    XBT_DEBUG("SIMIX_comm_finish: synchro state = %d", (int)synchro->state);

    /* Check out for errors */

    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
      SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
    } else {
      switch (comm->state) {

        case SIMIX_DONE:
          XBT_DEBUG("Communication %p complete!", synchro.get());
          SIMIX_comm_copy_data(synchro);
          break;

        case SIMIX_SRC_TIMEOUT:
          SMX_EXCEPTION(simcall->issuer, timeout_error, 0, "Communication timeouted because of sender");
          break;

        case SIMIX_DST_TIMEOUT:
          SMX_EXCEPTION(simcall->issuer, timeout_error, 0, "Communication timeouted because of receiver");
          break;

        case SIMIX_SRC_HOST_FAILURE:
          if (simcall->issuer == comm->src_proc)
            simcall->issuer->context->iwannadie = 1;
          else
            SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
          break;

        case SIMIX_DST_HOST_FAILURE:
          if (simcall->issuer == comm->dst_proc)
            simcall->issuer->context->iwannadie = 1;
          else
            SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
          break;

        case SIMIX_LINK_FAILURE:
          XBT_DEBUG("Link failure in synchro %p between '%s' and '%s': posting an exception to the issuer: %s (%p) "
                    "detached:%d",
                    synchro.get(), comm->src_proc ? comm->src_proc->host->getCname() : nullptr,
                    comm->dst_proc ? comm->dst_proc->host->getCname() : nullptr, simcall->issuer->getCname(),
                    simcall->issuer, comm->detached);
          if (comm->src_proc == simcall->issuer) {
            XBT_DEBUG("I'm source");
          } else if (comm->dst_proc == simcall->issuer) {
            XBT_DEBUG("I'm dest");
          } else {
            XBT_DEBUG("I'm neither source nor dest");
          }
          SMX_EXCEPTION(simcall->issuer, network_error, 0, "Link failure");
          break;

        case SIMIX_CANCELED:
          if (simcall->issuer == comm->dst_proc)
            SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Communication canceled by the sender");
          else
            SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Communication canceled by the receiver");
          break;

        default:
          xbt_die("Unexpected synchro state in SIMIX_comm_finish: %d", (int)synchro->state);
      }
    }

    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer->exception) {
      // In order to modify the exception we have to rethrow it:
      try {
        std::rethrow_exception(simcall->issuer->exception);
      }
      catch(xbt_ex& e) {
        if (simcall->call == SIMCALL_COMM_WAITANY) {
          e.value = xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &synchro);
        }
        else if (simcall->call == SIMCALL_COMM_TESTANY) {
          e.value = -1;
          auto comms = simcall_comm_testany__get__comms(simcall);
          auto count = simcall_comm_testany__get__count(simcall);
          auto element = std::find(comms, comms + count, synchro);
          if (element == comms + count)
            e.value = -1;
          else
            e.value = element - comms;
        }
        simcall->issuer->exception = std::make_exception_ptr(e);
      }
      catch(...) {
        // Nothing to do
      }
    }

    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro = nullptr;
    simcall->issuer->comms.remove(synchro);
    if(comm->detached){
      if(simcall->issuer == comm->src_proc){
        if(comm->dst_proc)
          comm->dst_proc->comms.remove(synchro);
      }
      else if(simcall->issuer == comm->dst_proc){
        if(comm->src_proc)
          comm->src_proc->comms.remove(synchro);
      }
      else{
        comm->dst_proc->comms.remove(synchro);
        comm->src_proc->comms.remove(synchro);
      }
    }

    SIMIX_simcall_answer(simcall);
  }
}

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_comm_copy_data_callback) (smx_activity_t, void*, size_t) = &SIMIX_comm_copy_pointer_callback;

void SIMIX_comm_set_copy_data_callback(void (*callback) (smx_activity_t, void*, size_t))
{
  SIMIX_comm_copy_data_callback = callback;
}

void SIMIX_comm_copy_pointer_callback(smx_activity_t synchro, void* buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  xbt_assert((buff_size == sizeof(void *)), "Cannot copy %zu bytes: must be sizeof(void*)", buff_size);
  *(void **) (comm->dst_buff) = buff;
}

void SIMIX_comm_copy_buffer_callback(smx_activity_t synchro, void* buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  XBT_DEBUG("Copy the data over");
  memcpy(comm->dst_buff, buff, buff_size);
  if (comm->detached) { // if this is a detached send, the source buffer was duplicated by SMPI sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    comm->src_buff = nullptr;
  }
}

/**
 *  @brief Copy the communication data from the sender's buffer to the receiver's one
 *  @param synchro The communication
 */
void SIMIX_comm_copy_data(smx_activity_t synchro)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  size_t buff_size = comm->src_buff_size;
  /* If there is no data to copy then return */
  if (not comm->src_buff || not comm->dst_buff || comm->copied)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)", comm.get(),
            comm->src_proc ? comm->src_proc->host->getCname() : "a finished process", comm->src_buff,
            comm->dst_proc ? comm->dst_proc->host->getCname() : "a finished process", comm->dst_buff, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (comm->dst_buff_size)
    buff_size = std::min(buff_size, *(comm->dst_buff_size));

  /* Update the receiver's buffer size to the copied amount */
  if (comm->dst_buff_size)
    *comm->dst_buff_size = buff_size;

  if (buff_size > 0){
      if(comm->copy_data_fun)
        comm->copy_data_fun (comm, comm->src_buff, buff_size);
      else
        SIMIX_comm_copy_data_callback (comm, comm->src_buff, buff_size);
  }

  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  comm->copied = 1;
}
